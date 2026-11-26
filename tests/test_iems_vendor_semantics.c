#include <cmocka.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>

#include "iems_vendor_semantics.h"

#define FAKE_MAX_ENTRIES 32

typedef struct {
    const char *tag;
    double value;
} fake_entry;

typedef struct {
    fake_entry writes[FAKE_MAX_ENTRIES];
    int write_count;
    fake_entry reads[FAKE_MAX_ENTRIES];
    int read_count;
    fake_entry presets[FAKE_MAX_ENTRIES];
    int preset_count;
    int fail_write;
} fake_io;

static int fake_read_tag(void *ctx, const char *dev_id, const char *tag_name, double *value) {
    fake_io *fake = ctx;
    (void)dev_id;
    assert_non_null(fake);
    assert_non_null(tag_name);
    assert_true(fake->read_count < FAKE_MAX_ENTRIES);

    int slot = fake->read_count;
    fake->reads[slot].tag = tag_name;
    fake->reads[slot].value = 0.0;
    fake->read_count = slot + 1;

    for (int i = 0; i < fake->preset_count; i++) {
        if (strcmp(fake->presets[i].tag, tag_name) == 0) {
            *value = fake->presets[i].value;
            fake->reads[slot].value = *value;
            return 0;
        }
    }
    return -1;
}

static int fake_write_tag(void *ctx, const char *dev_id, const char *tag_name, double value) {
    fake_io *fake = ctx;
    (void)dev_id;
    assert_non_null(fake);
    assert_non_null(tag_name);
    if (fake->fail_write != 0) {
        return fake->fail_write;
    }
    assert_true(fake->write_count < FAKE_MAX_ENTRIES);
    fake->writes[fake->write_count].tag = tag_name;
    fake->writes[fake->write_count].value = value;
    fake->write_count++;
    return 0;
}

static const iems_vendor_io fake_io_port = {
    .read_tag = fake_read_tag,
    .write_tag = fake_write_tag,
};

static void fake_preset(fake_io *fake, const char *tag, double value) {
    for (int i = 0; i < fake->preset_count; i++) {
        if (strcmp(fake->presets[i].tag, tag) == 0) {
            fake->presets[i].value = value;
            return;
        }
    }
    assert_true(fake->preset_count < FAKE_MAX_ENTRIES);
    fake->presets[fake->preset_count].tag = tag;
    fake->presets[fake->preset_count].value = value;
    fake->preset_count++;
}

static const iems_vendor_semantics *bind_device(iems_vendor_device *dev, fake_io *fake,
                                                const char *compatible) {
    int index = -1;
    const iems_vendor_semantics *ops = iems_vendor_registry_lookup(compatible, &index);
    assert_non_null(ops);
    dev->io = &fake_io_port;
    dev->ctx = fake;
    dev->dev_id = "dev-1";
    dev->compatible_index = index;
    return ops;
}

static const fake_entry *find_write(const fake_io *fake, const char *tag) {
    for (int i = 0; i < fake->write_count; i++) {
        if (strcmp(fake->writes[i].tag, tag) == 0) {
            return &fake->writes[i];
        }
    }
    return NULL;
}

static void assert_close(double actual, double expected) {
    assert_true(fabs(actual - expected) < 1e-9);
}

static void print_writes(const char *strategy, const fake_io *fake) {
    print_message("%s write sequence:\n", strategy);
    for (int i = 0; i < fake->write_count; i++) {
        print_message("  [%d] %s = %g\n", i, fake->writes[i].tag, fake->writes[i].value);
    }
}

static void test_registry_aliases_and_index(void **state) {
    (void)state;
    int index = -2;
    const iems_vendor_semantics *alias0 = iems_vendor_registry_lookup("pcs_alias_index", &index);
    assert_non_null(alias0);
    assert_int_equal(index, 0);

    const iems_vendor_semantics *alias1 =
        iems_vendor_registry_lookup("pcs_alias_index-3p4w", &index);
    assert_non_null(alias1);
    assert_int_equal(index, 1);
    assert_ptr_equal(alias0, alias1);

    const iems_vendor_semantics *alias2 = iems_vendor_registry_lookup("pcs_alias_index_x3", &index);
    assert_non_null(alias2);
    assert_int_equal(index, 2);
    assert_ptr_equal(alias0, alias2);

    const iems_vendor_semantics *alias3 = iems_vendor_registry_lookup("pcs_alias_index_x4", &index);
    assert_non_null(alias3);
    assert_int_equal(index, 3);
    assert_ptr_equal(alias0, alias3);

    assert_null(iems_vendor_registry_lookup("no_such_adapter", &index));
    assert_int_equal(index, -1);
    assert_non_null(iems_vendor_registry_lookup("pcs_split3", NULL));
    assert_null(iems_vendor_registry_lookup(NULL, &index));
    assert_int_equal(index, -1);

    static const char *const all_strategies[] = {
        "pcs_standard",   "pcs_split3",     "pcs_split3-3p4w", "pcs_onoff_01",
        "pcs_onoff_00ff", "pcs_onoff_5555", "pcs_int_double",  "mppt_sum4",
    };
    for (size_t i = 0; i < sizeof(all_strategies) / sizeof(all_strategies[0]); i++) {
        assert_non_null(iems_vendor_registry_lookup(all_strategies[i], NULL));
    }
}

static void test_standard_pcs(void **state) {
    (void)state;
    fake_io fake = {0};
    iems_vendor_device dev;
    const iems_vendor_semantics *ops = bind_device(&dev, &fake, "pcs_standard");
    assert_int_equal(ops->init(&dev, NULL), 0);

    assert_int_equal(ops->set_power(&dev, 12.5), 0);
    assert_int_equal(ops->set_repower(&dev, -4.25), 0);
    assert_int_equal(ops->set_mode(&dev, 3), 0);
    assert_int_equal(ops->set_onoff(&dev, IEMS_VENDOR_ON), 0);
    assert_int_equal(ops->set_connect(&dev, IEMS_VENDOR_CONNECT_GRID), 0);
    assert_int_equal(fake.write_count, 5);
    assert_string_equal(fake.writes[0].tag, "OutPowerSet");
    assert_close(fake.writes[0].value, 12.5);
    assert_string_equal(fake.writes[1].tag, "OutReactivePower");
    assert_close(fake.writes[1].value, -4.25);
    assert_string_equal(fake.writes[2].tag, "WorkMode");
    assert_close(fake.writes[2].value, 3.0);
    assert_string_equal(fake.writes[3].tag, "StopCtr");
    assert_close(fake.writes[3].value, 1.0);
    assert_string_equal(fake.writes[4].tag, "setConnect");
    assert_close(fake.writes[4].value, 0.0);
    print_writes("pcs_standard", &fake);

    fake_preset(&fake, "OutPowerSet", 7.5);
    double power = 0.0;
    assert_int_equal(ops->get_power(&dev, &power), 0);
    assert_close(power, 7.5);

    int onoff = -1;
    fake_preset(&fake, "StopCtr", 1.0);
    assert_int_equal(ops->get_onoff(&dev, &onoff), 0);
    assert_int_equal(onoff, IEMS_VENDOR_ON);
    fake_preset(&fake, "StopCtr", 2.0);
    assert_int_equal(ops->get_onoff(&dev, &onoff), -1);

    int mode = -1;
    fake_preset(&fake, "WorkMode", 5.0);
    assert_int_equal(ops->get_mode(&dev, &mode), 0);
    assert_int_equal(mode, 5);

    int connect = -1;
    fake_preset(&fake, "setConnect", 0.0);
    assert_int_equal(ops->get_connect(&dev, &connect), 0);
    assert_int_equal(connect, 0);

    // v3 把 get_mode/get_connect 误派到 get_onoff；扩展接口必须按函数名派发。
    fake.write_count = 0;
    assert_int_equal(ops->set_value(&dev, IEMS_VENDOR_FN_SET_POWER, 3.0), 0);
    assert_int_equal(fake.write_count, 1);
    assert_string_equal(fake.writes[0].tag, "OutPowerSet");
    assert_int_equal(ops->set_value(&dev, "unknown", 0.0), -1);
    assert_int_equal(ops->get_int_value(&dev, IEMS_VENDOR_FN_GET_MODE, &mode), 0);
    assert_int_equal(mode, 5);
    assert_int_equal(ops->get_int_value(&dev, "unknown", &mode), -1);
    assert_int_equal(ops->get_double_value(&dev, "unknown", &power), -1);

    fake.fail_write = -1;
    assert_int_equal(ops->set_power(&dev, 1.0), -1);
}

static void test_coepo_vendor(void **state) {
    (void)state;
    fake_io fake = {0};
    iems_vendor_device dev;
    const iems_vendor_semantics *ops = bind_device(&dev, &fake, "coepo");
    assert_int_equal(dev.compatible_index, 0);
    assert_true(ops != iems_vendor_registry_lookup("pcs_standard", NULL));
    assert_int_equal(ops->init(&dev, NULL), 0);

    assert_int_equal(ops->set_power(&dev, 12.5), 0);
    assert_int_equal(ops->set_repower(&dev, -4.25), 0);
    assert_int_equal(ops->set_mode(&dev, 3), 0);
    assert_int_equal(ops->set_onoff(&dev, IEMS_VENDOR_ON), 0);
    assert_int_equal(ops->set_connect(&dev, IEMS_VENDOR_CONNECT_GRID), 0);
    assert_int_equal(ops->set_connect(&dev, IEMS_VENDOR_DISCONNECT_GRID), 0);
    assert_int_equal(fake.write_count, 6);
    assert_string_equal(fake.writes[0].tag, "OutPowerSet");
    assert_close(fake.writes[0].value, 12.5);
    assert_string_equal(fake.writes[1].tag, "OutReactivePower");
    assert_close(fake.writes[1].value, -4.25);
    assert_string_equal(fake.writes[2].tag, "WorkMode");
    assert_close(fake.writes[2].value, 3.0);
    assert_string_equal(fake.writes[3].tag, "StopCtr");
    assert_close(fake.writes[3].value, 1.0);
    assert_string_equal(fake.writes[4].tag, "setConnect");
    assert_close(fake.writes[4].value, 0.0);
    assert_close(fake.writes[5].value, 1.0);
    print_writes("coepo (5-function)", &fake);

    double power = 0.0;
    fake_preset(&fake, "OutPowerSet", 7.5);
    assert_int_equal(ops->get_power(&dev, &power), 0);
    assert_close(power, 7.5);

    double repower = 0.0;
    fake_preset(&fake, "OutReactivePower", -2.25);
    assert_int_equal(ops->get_repower(&dev, &repower), 0);
    assert_close(repower, -2.25);

    int onoff = -1;
    fake_preset(&fake, "StopCtr", 1.0);
    assert_int_equal(ops->get_onoff(&dev, &onoff), 0);
    assert_int_equal(onoff, IEMS_VENDOR_ON);

    int mode = -1;
    fake_preset(&fake, "WorkMode", 5.0);
    assert_int_equal(ops->get_mode(&dev, &mode), 0);
    assert_int_equal(mode, 5);

    // v3 写 setConnect 并网 0/离网 1，读却返回原值；读侧保持 v3 不对称。
    int connect = -1;
    fake_preset(&fake, "setConnect", 0.0);
    assert_int_equal(ops->get_connect(&dev, &connect), 0);
    assert_int_equal(connect, 0);

    // v3 把 get_mode/get_connect 误派到 get_onoff；扩展接口必须读同名 tag。
    fake.read_count = 0;
    assert_int_equal(ops->get_int_value(&dev, IEMS_VENDOR_FN_GET_MODE, &mode), 0);
    assert_string_equal(fake.reads[0].tag, "WorkMode");
    assert_int_equal(ops->get_int_value(&dev, IEMS_VENDOR_FN_GET_CONNECT, &connect), 0);
    assert_string_equal(fake.reads[1].tag, "setConnect");
    assert_int_equal(ops->get_int_value(&dev, IEMS_VENDOR_FN_GET_ONOFF, &onoff), 0);
    assert_string_equal(fake.reads[2].tag, "StopCtr");
}

static void test_alias_index_phase_mode(void **state) {
    (void)state;
    fake_io split = {0};
    iems_vendor_device split_dev;
    const iems_vendor_semantics *ops = bind_device(&split_dev, &split, "pcs_alias_index");
    assert_int_equal(split_dev.compatible_index, 0);

    assert_int_equal(ops->set_power(&split_dev, 9.0), 0);
    assert_int_equal(split.write_count, 3);
    assert_string_equal(split.writes[0].tag, "ASplitPhaseActPowSet");
    assert_string_equal(split.writes[1].tag, "BSplitPhaseActPowSet");
    assert_string_equal(split.writes[2].tag, "CSplitPhaseActPowSet");
    for (int i = 0; i < 3; i++) {
        assert_close(split.writes[i].value, 3.0);
    }
    print_writes("pcs_alias_index (index 0, split)", &split);

    split.write_count = 0;
    assert_int_equal(ops->set_repower(&split_dev, 6.0), 0);
    assert_int_equal(split.write_count, 3);
    assert_string_equal(split.writes[0].tag, "ASplitPhaseReactPowSet");
    for (int i = 0; i < 3; i++) {
        assert_close(split.writes[i].value, 2.0);
    }
    print_writes("pcs_alias_index (index 0, split repower)", &split);

    fake_io single = {0};
    iems_vendor_device single_dev;
    ops = bind_device(&single_dev, &single, "pcs_alias_index-3p4w");
    assert_int_equal(single_dev.compatible_index, 1);
    assert_int_equal(ops->set_power(&single_dev, 9.0), 0);
    assert_int_equal(single.write_count, 1);
    assert_string_equal(single.writes[0].tag, "ActPowSet");
    assert_close(single.writes[0].value, 9.0);
    print_writes("pcs_alias_index-3p4w (index 1, single)", &single);

    fake_io x3 = {0};
    iems_vendor_device x3_dev;
    ops = bind_device(&x3_dev, &x3, "pcs_alias_index_x3");
    assert_int_equal(x3_dev.compatible_index, 2);
    assert_int_equal(ops->set_power(&x3_dev, 9.0), 0);
    assert_int_equal(x3.write_count, 1);

    fake_io x4 = {0};
    iems_vendor_device x4_dev;
    ops = bind_device(&x4_dev, &x4, "pcs_alias_index_x4");
    assert_int_equal(x4_dev.compatible_index, 3);
    assert_int_equal(ops->set_power(&x4_dev, 9.0), 0);
    assert_int_equal(x4.write_count, 3);

    x4.write_count = 0;
    assert_int_equal(ops->set_onoff(&x4_dev, IEMS_VENDOR_ON), 0);
    assert_string_equal(x4.writes[0].tag, "FaultRun");
    assert_close(x4.writes[0].value, 1.0);
    assert_int_equal(ops->set_onoff(&x4_dev, IEMS_VENDOR_OFF), 0);
    assert_string_equal(x4.writes[1].tag, "FaultStop");
    assert_close(x4.writes[1].value, 1.0);
    assert_int_equal(ops->set_onoff(&x4_dev, IEMS_VENDOR_STANDBY), -1);

    int onoff = -1;
    fake_preset(&x4, "RunStatu", 1.0);
    assert_int_equal(ops->get_onoff(&x4_dev, &onoff), 0);
    assert_int_equal(onoff, IEMS_VENDOR_ON);
    fake_preset(&x4, "RunStatu", 0.0);
    fake_preset(&x4, "Stoppage", 1.0);
    assert_int_equal(ops->get_onoff(&x4_dev, &onoff), 0);
    assert_int_equal(onoff, IEMS_VENDOR_OFF);
    fake_preset(&x4, "Stoppage", 0.0);
    assert_int_equal(ops->get_onoff(&x4_dev, &onoff), 0);
    assert_int_equal(onoff, IEMS_VENDOR_STANDBY);

    x4.write_count = 0;
    assert_int_equal(ops->set_connect(&x4_dev, IEMS_VENDOR_CONNECT_GRID), 0);
    assert_string_equal(x4.writes[0].tag, "SetGridModel");
    assert_close(x4.writes[0].value, 0.0);
    assert_int_equal(ops->set_connect(&x4_dev, IEMS_VENDOR_DISCONNECT_GRID), 0);
    assert_close(x4.writes[1].value, 1.0);

    int mode = 0;
    int connect = 0;
    assert_int_equal(ops->set_mode(&x4_dev, 1), -1);
    assert_int_equal(ops->get_mode(&x4_dev, &mode), -1);
    assert_int_equal(ops->get_connect(&x4_dev, &connect), -1);
}

static void test_inpower_vendor(void **state) {
    (void)state;
    static const char *const aliases[] = {"inpower", "inpower-3p4w", "inpower_x3", "inpower_x4"};
    const iems_vendor_semantics *vendor = NULL;
    for (int index = 0; index < 4; index++) {
        int found = -1;
        const iems_vendor_semantics *ops = iems_vendor_registry_lookup(aliases[index], &found);
        assert_non_null(ops);
        assert_int_equal(found, index);
        if (vendor == NULL) {
            vendor = ops;
        }
        assert_ptr_equal(ops, vendor);
    }
    assert_true(vendor != iems_vendor_registry_lookup("pcs_alias_index", NULL));

    fake_io split = {0};
    iems_vendor_device split_dev;
    const iems_vendor_semantics *ops = bind_device(&split_dev, &split, "inpower");
    assert_int_equal(split_dev.compatible_index, 0);
    assert_int_equal(ops->set_power(&split_dev, 9.0), 0);
    assert_int_equal(split.write_count, 3);
    assert_string_equal(split.writes[0].tag, "ASplitPhaseActPowSet");
    assert_string_equal(split.writes[1].tag, "BSplitPhaseActPowSet");
    assert_string_equal(split.writes[2].tag, "CSplitPhaseActPowSet");
    for (int i = 0; i < 3; i++) {
        assert_close(split.writes[i].value, 3.0);
    }
    assert_null(find_write(&split, "ActPowSet"));
    print_writes("inpower (index 0, split power)", &split);

    split.write_count = 0;
    assert_int_equal(ops->set_repower(&split_dev, 6.0), 0);
    assert_int_equal(split.write_count, 3);
    assert_string_equal(split.writes[0].tag, "ASplitPhaseReactPowSet");
    assert_string_equal(split.writes[1].tag, "BSplitPhaseReactPowSet");
    assert_string_equal(split.writes[2].tag, "CSplitPhaseReactPowSet");
    for (int i = 0; i < 3; i++) {
        assert_close(split.writes[i].value, 2.0);
    }
    print_writes("inpower (index 0, split repower)", &split);

    fake_io single = {0};
    iems_vendor_device single_dev;
    ops = bind_device(&single_dev, &single, "inpower-3p4w");
    assert_int_equal(single_dev.compatible_index, 1);
    assert_int_equal(ops->set_power(&single_dev, 9.0), 0);
    assert_int_equal(ops->set_repower(&single_dev, 6.0), 0);
    assert_int_equal(single.write_count, 2);
    assert_string_equal(single.writes[0].tag, "ActPowSet");
    assert_close(single.writes[0].value, 9.0);
    assert_string_equal(single.writes[1].tag, "ReactPowSet");
    assert_close(single.writes[1].value, 6.0);
    print_writes("inpower-3p4w (index 1, single)", &single);

    fake_io x3 = {0};
    iems_vendor_device x3_dev;
    ops = bind_device(&x3_dev, &x3, "inpower_x3");
    assert_int_equal(x3_dev.compatible_index, 2);
    assert_int_equal(ops->set_power(&x3_dev, 9.0), 0);
    assert_int_equal(x3.write_count, 1);
    assert_string_equal(x3.writes[0].tag, "ActPowSet");
    print_writes("inpower_x3 (index 2, single)", &x3);

    fake_io x4 = {0};
    iems_vendor_device x4_dev;
    ops = bind_device(&x4_dev, &x4, "inpower_x4");
    assert_int_equal(x4_dev.compatible_index, 3);
    assert_int_equal(ops->set_power(&x4_dev, 9.0), 0);
    assert_int_equal(x4.write_count, 3);
    assert_string_equal(x4.writes[0].tag, "ASplitPhaseActPowSet");
    assert_close(x4.writes[0].value, 3.0);
    print_writes("inpower_x4 (index 3, split power)", &x4);

    x4.write_count = 0;
    assert_int_equal(ops->set_onoff(&x4_dev, IEMS_VENDOR_ON), 0);
    assert_string_equal(x4.writes[0].tag, "FaultRun");
    assert_close(x4.writes[0].value, 1.0);
    assert_int_equal(ops->set_onoff(&x4_dev, IEMS_VENDOR_OFF), 0);
    assert_string_equal(x4.writes[1].tag, "FaultStop");
    assert_close(x4.writes[1].value, 1.0);
    assert_int_equal(ops->set_onoff(&x4_dev, IEMS_VENDOR_STANDBY), -1);
    assert_int_equal(x4.write_count, 2);
    print_writes("inpower (on/off)", &x4);

    int onoff = -1;
    fake_preset(&x4, "RunStatu", 1.0);
    assert_int_equal(ops->get_onoff(&x4_dev, &onoff), 0);
    assert_int_equal(onoff, IEMS_VENDOR_ON);
    fake_preset(&x4, "RunStatu", 0.0);
    fake_preset(&x4, "Stoppage", 1.0);
    assert_int_equal(ops->get_onoff(&x4_dev, &onoff), 0);
    assert_int_equal(onoff, IEMS_VENDOR_OFF);
    fake_preset(&x4, "Stoppage", 0.0);
    assert_int_equal(ops->get_onoff(&x4_dev, &onoff), 0);
    assert_int_equal(onoff, IEMS_VENDOR_STANDBY);

    x4.write_count = 0;
    assert_int_equal(ops->set_connect(&x4_dev, IEMS_VENDOR_CONNECT_GRID), 0);
    assert_string_equal(x4.writes[0].tag, "SetGridModel");
    assert_close(x4.writes[0].value, 0.0);
    assert_int_equal(ops->set_connect(&x4_dev, IEMS_VENDOR_DISCONNECT_GRID), 0);
    assert_string_equal(x4.writes[1].tag, "SetGridModel");
    assert_close(x4.writes[1].value, 1.0);
    assert_int_equal(ops->set_connect(&x4_dev, 9), -1);
    assert_int_equal(x4.write_count, 2);
    print_writes("inpower (connect)", &x4);

    // v3 的 set_mode 空实现、get_mode/get_connect 不写输出：这里返回 -1 且不碰 I/O。
    int mode = 0;
    int connect = 0;
    int writes_before = x4.write_count;
    int reads_before = x4.read_count;
    assert_int_equal(ops->set_mode(&x4_dev, 1), -1);
    assert_int_equal(ops->get_mode(&x4_dev, &mode), -1);
    assert_int_equal(ops->get_connect(&x4_dev, &connect), -1);
    assert_int_equal(x4.write_count, writes_before);
    assert_int_equal(x4.read_count, reads_before);

    double power = 0.0;
    fake_preset(&x4, "ActivePower", 5.5);
    assert_int_equal(ops->get_power(&x4_dev, &power), 0);
    assert_close(power, 5.5);
    double repower = 0.0;
    fake_preset(&x4, "ReactivePower", -1.5);
    assert_int_equal(ops->get_repower(&x4_dev, &repower), 0);
    assert_close(repower, -1.5);
}

static void test_gsstes_vendor(void **state) {
    (void)state;
    fake_io fake = {0};
    iems_vendor_device dev;
    const iems_vendor_semantics *ops = bind_device(&dev, &fake, "gsstes");
    assert_int_equal(dev.compatible_index, 0);
    assert_true(ops != iems_vendor_registry_lookup("pcs_standard", NULL));
    assert_true(ops != iems_vendor_registry_lookup("pcs_int_double", NULL));
    assert_int_equal(ops->init(&dev, NULL), 0);

    assert_int_equal(ops->set_power(&dev, 12.5), 0);
    assert_int_equal(ops->set_repower(&dev, -4.25), 0);
    assert_int_equal(ops->set_mode(&dev, 3), 0);
    assert_int_equal(ops->set_onoff(&dev, IEMS_VENDOR_ON), 0);
    assert_int_equal(ops->set_onoff(&dev, IEMS_VENDOR_OFF), 0);
    assert_int_equal(ops->set_connect(&dev, IEMS_VENDOR_CONNECT_GRID), 0);
    assert_int_equal(ops->set_connect(&dev, IEMS_VENDOR_DISCONNECT_GRID), 0);
    assert_int_equal(ops->set_onoff(&dev, IEMS_VENDOR_STANDBY), -1);
    assert_int_equal(fake.write_count, 7);
    assert_string_equal(fake.writes[0].tag, "ActPowSet");
    assert_close(fake.writes[0].value, 12.5);
    assert_string_equal(fake.writes[1].tag, "ReactPowSet");
    assert_close(fake.writes[1].value, -4.25);
    assert_string_equal(fake.writes[2].tag, "WorkMode");
    assert_close(fake.writes[2].value, 3.0);
    assert_string_equal(fake.writes[3].tag, "StartStopCtr");
    assert_close(fake.writes[3].value, 0xff00);
    assert_string_equal(fake.writes[4].tag, "StartStopCtr");
    assert_close(fake.writes[4].value, 0x00ff);
    assert_string_equal(fake.writes[5].tag, "setConnect");
    assert_close(fake.writes[5].value, 1.0);
    assert_string_equal(fake.writes[6].tag, "setConnect");
    assert_close(fake.writes[6].value, 0.0);
    print_writes("gsstes (5-function)", &fake);

    double power = 0.0;
    fake_preset(&fake, "ActivePower", 7.9);
    assert_int_equal(ops->get_power(&dev, &power), 0);
    assert_close(power, 7.0);
    double repower = 0.0;
    fake_preset(&fake, "ReactivePower", -7.9);
    assert_int_equal(ops->get_repower(&dev, &repower), 0);
    assert_close(repower, -7.0);

    int onoff = -1;
    fake_preset(&fake, "StartStopCtr", 0xff00);
    assert_int_equal(ops->get_onoff(&dev, &onoff), 0);
    assert_int_equal(onoff, IEMS_VENDOR_ON);
    fake_preset(&fake, "StartStopCtr", 0x00ff);
    assert_int_equal(ops->get_onoff(&dev, &onoff), 0);
    assert_int_equal(onoff, IEMS_VENDOR_OFF);
    fake_preset(&fake, "StartStopCtr", 1.0);
    assert_int_equal(ops->get_onoff(&dev, &onoff), -1);

    int mode = -1;
    fake_preset(&fake, "WorkMode", 5.0);
    assert_int_equal(ops->get_mode(&dev, &mode), 0);
    assert_int_equal(mode, 5);

    int connect = -1;
    fake_preset(&fake, "setConnect", 1.0);
    assert_int_equal(ops->get_connect(&dev, &connect), 0);
    assert_int_equal(connect, IEMS_VENDOR_CONNECT_GRID);

    // v3 扩展接口把 get_mode/get_connect 误派到 get_onoff；这里按同名 tag 读。
    fake.read_count = 0;
    assert_int_equal(ops->get_int_value(&dev, IEMS_VENDOR_FN_GET_MODE, &mode), 0);
    assert_string_equal(fake.reads[0].tag, "WorkMode");
    assert_int_equal(ops->get_int_value(&dev, IEMS_VENDOR_FN_GET_CONNECT, &connect), 0);
    assert_string_equal(fake.reads[1].tag, "setConnect");
}

static void test_kelong_vendor(void **state) {
    (void)state;
    fake_io fake = {0};
    iems_vendor_device dev;
    const iems_vendor_semantics *ops = bind_device(&dev, &fake, "kelong");
    assert_int_equal(dev.compatible_index, 0);
    assert_true(ops != iems_vendor_registry_lookup("pcs_onoff_01", NULL));
    assert_true(ops != iems_vendor_registry_lookup("pcs_int_double", NULL));
    assert_int_equal(ops->init(&dev, NULL), 0);

    assert_int_equal(ops->set_power(&dev, 12.5), 0);
    assert_int_equal(ops->set_repower(&dev, -4.25), 0);
    assert_int_equal(ops->set_mode(&dev, 3), 0);
    assert_int_equal(ops->set_onoff(&dev, IEMS_VENDOR_ON), 0);
    assert_int_equal(ops->set_onoff(&dev, IEMS_VENDOR_OFF), 0);
    assert_int_equal(ops->set_connect(&dev, IEMS_VENDOR_CONNECT_GRID), 0);
    assert_int_equal(ops->set_connect(&dev, IEMS_VENDOR_DISCONNECT_GRID), 0);
    assert_int_equal(fake.write_count, 7);
    assert_string_equal(fake.writes[0].tag, "ActPowSet");
    assert_close(fake.writes[0].value, 12.5);
    assert_string_equal(fake.writes[1].tag, "ReactPowSet");
    assert_close(fake.writes[1].value, -4.25);
    assert_string_equal(fake.writes[2].tag, "WorkMode");
    assert_close(fake.writes[2].value, 3.0);
    assert_string_equal(fake.writes[3].tag, "StartStopCtr");
    assert_close(fake.writes[3].value, 1.0);
    assert_string_equal(fake.writes[4].tag, "StartStopCtr");
    assert_close(fake.writes[4].value, 0.0);
    assert_string_equal(fake.writes[5].tag, "setConnect");
    assert_close(fake.writes[5].value, 1.0);
    assert_string_equal(fake.writes[6].tag, "setConnect");
    assert_close(fake.writes[6].value, 0.0);
    print_writes("kelong (5-function)", &fake);

    // v3 读回写侧 tag（ActPowSet/ReactPowSet），并用 int 访问器截断。
    double power = 0.0;
    fake_preset(&fake, "ActPowSet", 7.9);
    assert_int_equal(ops->get_power(&dev, &power), 0);
    assert_close(power, 7.0);
    double repower = 0.0;
    fake_preset(&fake, "ReactPowSet", -7.9);
    assert_int_equal(ops->get_repower(&dev, &repower), 0);
    assert_close(repower, -7.0);

    int onoff = -1;
    fake_preset(&fake, "StartStopCtr", 1.0);
    assert_int_equal(ops->get_onoff(&dev, &onoff), 0);
    assert_int_equal(onoff, IEMS_VENDOR_ON);
    fake_preset(&fake, "StartStopCtr", 0.0);
    assert_int_equal(ops->get_onoff(&dev, &onoff), 0);
    assert_int_equal(onoff, IEMS_VENDOR_OFF);
    fake_preset(&fake, "StartStopCtr", 2.0);
    assert_int_equal(ops->get_onoff(&dev, &onoff), -1);

    int mode = -1;
    fake_preset(&fake, "WorkMode", 5.0);
    assert_int_equal(ops->get_mode(&dev, &mode), 0);
    assert_int_equal(mode, 5);

    int connect = -1;
    fake_preset(&fake, "setConnect", 0.0);
    assert_int_equal(ops->get_connect(&dev, &connect), 0);
    assert_int_equal(connect, IEMS_VENDOR_DISCONNECT_GRID);

    // v3 扩展接口把 get_mode/get_connect 误派到 get_onoff；这里按同名 tag 读。
    fake.read_count = 0;
    assert_int_equal(ops->get_int_value(&dev, IEMS_VENDOR_FN_GET_MODE, &mode), 0);
    assert_string_equal(fake.reads[0].tag, "WorkMode");
    assert_int_equal(ops->get_int_value(&dev, IEMS_VENDOR_FN_GET_CONNECT, &connect), 0);
    assert_string_equal(fake.reads[1].tag, "setConnect");
}

static void test_xinhuasan_vendor(void **state) {
    (void)state;
    static const char *const aliases[] = {"xinhuasan", "xinhuasan-3p4w"};
    const iems_vendor_semantics *vendor = NULL;
    for (int index = 0; index < 2; index++) {
        int found = -1;
        const iems_vendor_semantics *ops = iems_vendor_registry_lookup(aliases[index], &found);
        assert_non_null(ops);
        assert_int_equal(found, index);
        if (vendor == NULL) {
            vendor = ops;
        }
        assert_ptr_equal(ops, vendor);
    }
    assert_true(vendor != iems_vendor_registry_lookup("pcs_split3", NULL));

    fake_io single = {0};
    iems_vendor_device single_dev;
    const iems_vendor_semantics *ops = bind_device(&single_dev, &single, "xinhuasan");
    assert_int_equal(single_dev.compatible_index, 0);
    assert_int_equal(ops->set_power(&single_dev, 9.0), 0);
    assert_int_equal(ops->set_repower(&single_dev, 6.0), 0);
    assert_int_equal(single.write_count, 2);
    assert_string_equal(single.writes[0].tag, "ActPowSet");
    assert_close(single.writes[0].value, 9.0);
    assert_string_equal(single.writes[1].tag, "ReactPowSet");
    assert_close(single.writes[1].value, 6.0);
    print_writes("xinhuasan (single)", &single);

    double power = 0.0;
    fake_preset(&single, "ActivePower", 8.5);
    assert_int_equal(ops->get_power(&single_dev, &power), 0);
    assert_close(power, 8.5);
    double repower = 0.0;
    fake_preset(&single, "ReactivePower", -1.5);
    assert_int_equal(ops->get_repower(&single_dev, &repower), 0);
    assert_close(repower, -1.5);

    fake_io split = {0};
    iems_vendor_device split_dev;
    ops = bind_device(&split_dev, &split, "xinhuasan-3p4w");
    assert_int_equal(split_dev.compatible_index, 1);
    assert_int_equal(ops->set_power(&split_dev, 9.0), 0);
    assert_int_equal(split.write_count, 3);
    assert_string_equal(split.writes[0].tag, "ASplitPhaseActPowSet");
    assert_string_equal(split.writes[1].tag, "BSplitPhaseActPowSet");
    assert_string_equal(split.writes[2].tag, "CSplitPhaseActPowSet");
    for (int i = 0; i < 3; i++) {
        assert_close(split.writes[i].value, 3.0);
    }
    assert_null(find_write(&split, "ActPowSet"));
    print_writes("xinhuasan-3p4w (power split)", &split);

    split.write_count = 0;
    assert_int_equal(ops->set_repower(&split_dev, 6.0), 0);
    assert_int_equal(split.write_count, 3);
    assert_string_equal(split.writes[0].tag, "ASplitPhaseReactPowSet");
    assert_string_equal(split.writes[1].tag, "BSplitPhaseReactPowSet");
    assert_string_equal(split.writes[2].tag, "CSplitPhaseReactPowSet");
    for (int i = 0; i < 3; i++) {
        assert_close(split.writes[i].value, 2.0);
    }
    assert_null(find_write(&split, "ReactPowSet"));
    print_writes("xinhuasan-3p4w (repower split)", &split);

    // v3 三相读逐相覆盖，结果等于 C 相，不是三相累加。
    split.read_count = 0;
    fake_preset(&split, "AphaseActPower", 1.0);
    fake_preset(&split, "BphaseActPower", 2.0);
    fake_preset(&split, "CphaseActPower", 3.0);
    assert_int_equal(ops->get_power(&split_dev, &power), 0);
    assert_close(power, 3.0);
    assert_int_equal(split.read_count, 3);
    assert_string_equal(split.reads[0].tag, "AphaseActPower");
    assert_string_equal(split.reads[1].tag, "BphaseActPower");
    assert_string_equal(split.reads[2].tag, "CphaseActPower");
    print_message("xinhuasan-3p4w get_power: A=1 B=2 C=3 -> %g\n", power);

    fake_preset(&split, "AphaseReactivePower", 10.0);
    fake_preset(&split, "BphaseReactivePower", 20.0);
    fake_preset(&split, "CphaseReactivePower", 30.0);
    assert_int_equal(ops->get_repower(&split_dev, &repower), 0);
    assert_close(repower, 30.0);
    print_message("xinhuasan-3p4w get_repower: A=10 B=20 C=30 -> %g\n", repower);

    split.write_count = 0;
    assert_int_equal(ops->set_mode(&split_dev, 4), 0);
    assert_int_equal(ops->set_onoff(&split_dev, IEMS_VENDOR_ON), 0);
    assert_int_equal(ops->set_onoff(&split_dev, IEMS_VENDOR_OFF), 0);
    assert_int_equal(ops->set_connect(&split_dev, IEMS_VENDOR_CONNECT_GRID), 0);
    assert_int_equal(ops->set_connect(&split_dev, IEMS_VENDOR_DISCONNECT_GRID), 0);
    assert_int_equal(split.write_count, 5);
    assert_string_equal(split.writes[0].tag, "WorkMode");
    assert_close(split.writes[0].value, 4.0);
    assert_string_equal(split.writes[1].tag, "PCSStart");
    assert_close(split.writes[1].value, 1.0);
    assert_string_equal(split.writes[2].tag, "PCSStop");
    assert_close(split.writes[2].value, 1.0);
    assert_string_equal(split.writes[3].tag, "SetConnect");
    assert_close(split.writes[3].value, 0.0);
    assert_string_equal(split.writes[4].tag, "SetConnect");
    assert_close(split.writes[4].value, 1.0);
    print_writes("xinhuasan-3p4w (mode/onoff/connect)", &split);

    int onoff = -1;
    fake_preset(&split, "Stoppage", 1.0);
    assert_int_equal(ops->get_onoff(&split_dev, &onoff), 0);
    assert_int_equal(onoff, IEMS_VENDOR_OFF);
    fake_preset(&split, "Stoppage", 0.0);
    assert_int_equal(ops->get_onoff(&split_dev, &onoff), 0);
    assert_int_equal(onoff, IEMS_VENDOR_ON);

    int mode = 0;
    fake_preset(&split, "RunStatu", 7.0);
    assert_int_equal(ops->get_mode(&split_dev, &mode), 0);
    assert_int_equal(mode, 7);

    int connect = -1;
    fake_preset(&split, "GridOnOff", 1.0);
    assert_int_equal(ops->get_connect(&split_dev, &connect), 0);
    assert_int_equal(connect, IEMS_VENDOR_DISCONNECT_GRID);
    fake_preset(&split, "GridOnOff", 0.0);
    assert_int_equal(ops->get_connect(&split_dev, &connect), 0);
    assert_int_equal(connect, IEMS_VENDOR_CONNECT_GRID);
}

static void test_sinexcel_vendor(void **state) {
    (void)state;
    fake_io fake = {0};
    iems_vendor_device dev;
    const iems_vendor_semantics *ops = bind_device(&dev, &fake, "sinexcel");
    assert_int_equal(dev.compatible_index, 0);
    assert_true(ops != iems_vendor_registry_lookup("pcs_standard", NULL));
    assert_int_equal(ops->init(&dev, NULL), 0);

    assert_int_equal(ops->set_power(&dev, 12.5), 0);
    assert_int_equal(ops->set_repower(&dev, -4.25), 0);
    assert_int_equal(ops->set_mode(&dev, 3), 0);
    assert_int_equal(ops->set_onoff(&dev, IEMS_VENDOR_ON), 0);
    assert_int_equal(ops->set_onoff(&dev, IEMS_VENDOR_OFF), 0);
    assert_int_equal(fake.write_count, 5);
    assert_string_equal(fake.writes[0].tag, "ActPowSet");
    assert_close(fake.writes[0].value, 12.5);
    assert_string_equal(fake.writes[1].tag, "ReactPowSet");
    assert_close(fake.writes[1].value, -4.25);
    assert_string_equal(fake.writes[2].tag, "WorkMode");
    assert_close(fake.writes[2].value, 3.0);
    assert_string_equal(fake.writes[3].tag, "TotalStart");
    assert_close(fake.writes[3].value, 1.0);
    assert_string_equal(fake.writes[4].tag, "TotalStop");
    assert_close(fake.writes[4].value, 1.0);
    print_writes("sinexcel", &fake);

    // v3 set_connect 是空实现（sinexcel.c:49-53）：这里返回 -1 且不碰 I/O。
    assert_int_equal(ops->set_connect(&dev, IEMS_VENDOR_CONNECT_GRID), -1);
    assert_int_equal(fake.write_count, 5);

    // v3 用 int 访问器读回写侧 setpoint tag（sinexcel.c:55-71）。
    double power = 0.0;
    fake_preset(&fake, "ActPowSet", 7.9);
    assert_int_equal(ops->get_power(&dev, &power), 0);
    assert_close(power, 7.0);
    double repower = 0.0;
    fake_preset(&fake, "ReactPowSet", -7.9);
    assert_int_equal(ops->get_repower(&dev, &repower), 0);
    assert_close(repower, -7.0);

    int mode = -1;
    fake_preset(&fake, "WorkMode", 5.0);
    assert_int_equal(ops->get_mode(&dev, &mode), 0);
    assert_int_equal(mode, 5);

    // RunStatu 只有 1 判开机，其余都是关机（sinexcel.c:73-86）。
    int onoff = -1;
    fake_preset(&fake, "RunStatu", 1.0);
    assert_int_equal(ops->get_onoff(&dev, &onoff), 0);
    assert_int_equal(onoff, IEMS_VENDOR_ON);
    fake_preset(&fake, "RunStatu", 0.0);
    assert_int_equal(ops->get_onoff(&dev, &onoff), 0);
    assert_int_equal(onoff, IEMS_VENDOR_OFF);

    int connect = -1;
    fake_preset(&fake, "setConnect", 1.0);
    assert_int_equal(ops->get_connect(&dev, &connect), 0);
    assert_int_equal(connect, 1);

    // v3 把 get_mode/get_connect 误派到 get_onoff；扩展接口读同名 tag。
    fake.read_count = 0;
    assert_int_equal(ops->get_int_value(&dev, IEMS_VENDOR_FN_GET_MODE, &mode), 0);
    assert_string_equal(fake.reads[0].tag, "WorkMode");
    assert_int_equal(ops->get_int_value(&dev, IEMS_VENDOR_FN_GET_CONNECT, &connect), 0);
    assert_string_equal(fake.reads[1].tag, "setConnect");
}

static void test_unsigned_pow_vendor(void **state) {
    (void)state;
    fake_io fake = {0};
    iems_vendor_device dev;
    const iems_vendor_semantics *ops = bind_device(&dev, &fake, "unsigned_pow");
    assert_int_equal(dev.compatible_index, 0);
    assert_true(ops != iems_vendor_registry_lookup("pcs_onoff_00ff", NULL));
    assert_int_equal(ops->init(&dev, NULL), 0);

    // 读不到 EnergyManageMode 就不能确定模式，直接失败且不写。
    assert_int_equal(ops->set_power(&dev, 5.0), -1);
    assert_int_equal(fake.write_count, 0);

    // 已在外部 EMS 模式（3）：只写方向指令与功率。
    fake_preset(&fake, "EnergyManageMode", 3.0);
    assert_int_equal(ops->set_power(&dev, 10.0), 0);
    assert_int_equal(fake.write_count, 2);
    assert_string_equal(fake.writes[0].tag, "BatteryChargeDischargeCmd");
    assert_close(fake.writes[0].value, 0xBB);
    assert_string_equal(fake.writes[1].tag, "ActPowSet");
    assert_close(fake.writes[1].value, 10.0);
    print_writes("unsigned_pow (discharge)", &fake);

    // 非外部 EMS 模式：先补写 EnergyManageMode=3，充电方向写 0xAA 与 |power|。
    fake.write_count = 0;
    fake_preset(&fake, "EnergyManageMode", 0.0);
    assert_int_equal(ops->set_power(&dev, -10.0), 0);
    assert_int_equal(fake.write_count, 3);
    assert_string_equal(fake.writes[0].tag, "EnergyManageMode");
    assert_close(fake.writes[0].value, 3.0);
    assert_string_equal(fake.writes[1].tag, "BatteryChargeDischargeCmd");
    assert_close(fake.writes[1].value, 0xAA);
    assert_string_equal(fake.writes[2].tag, "ActPowSet");
    assert_close(fake.writes[2].value, 10.0);
    print_writes("unsigned_pow (charge)", &fake);

    // 0 附近置零走 0xCC。
    fake.write_count = 0;
    fake_preset(&fake, "EnergyManageMode", 3.0);
    assert_int_equal(ops->set_power(&dev, 0.0005), 0);
    assert_int_equal(fake.write_count, 2);
    assert_close(fake.writes[0].value, 0xCC);
    assert_string_equal(fake.writes[1].tag, "ActPowSet");
    assert_close(fake.writes[1].value, 0.0);

    fake.write_count = 0;
    assert_int_equal(ops->set_repower(&dev, -4.25), 0);
    assert_int_equal(ops->set_mode(&dev, 3), 0);
    assert_int_equal(ops->set_onoff(&dev, IEMS_VENDOR_ON), 0);
    assert_int_equal(ops->set_onoff(&dev, IEMS_VENDOR_OFF), 0);
    assert_int_equal(ops->set_onoff(&dev, IEMS_VENDOR_STANDBY), -1);
    assert_int_equal(fake.write_count, 4);
    assert_string_equal(fake.writes[0].tag, "QT");
    assert_close(fake.writes[0].value, -4.25);
    assert_string_equal(fake.writes[1].tag, "WorkMode");
    assert_close(fake.writes[1].value, 3.0);
    assert_string_equal(fake.writes[2].tag, "StartStopCtr");
    assert_close(fake.writes[2].value, 207.0);
    assert_string_equal(fake.writes[3].tag, "StartStopCtr");
    assert_close(fake.writes[3].value, 206.0);

    fake.write_count = 0;
    assert_int_equal(ops->set_connect(&dev, IEMS_VENDOR_CONNECT_GRID), 0);
    assert_string_equal(fake.writes[0].tag, "GridMode");
    assert_close(fake.writes[0].value, 0xA5);
    assert_int_equal(ops->set_connect(&dev, IEMS_VENDOR_DISCONNECT_GRID), 0);
    assert_close(fake.writes[1].value, 0x55);
    // v3 对未定义 connect 静默返回 0；这里返回 -1 且不写。
    assert_int_equal(ops->set_connect(&dev, 2), -1);
    assert_int_equal(fake.write_count, 2);
    print_writes("unsigned_pow (connect)", &fake);

    // 功率/无功/模式读 int 访问器并显式截断。
    double power = 0.0;
    fake_preset(&fake, "ActivePower", 7.9);
    assert_int_equal(ops->get_power(&dev, &power), 0);
    assert_close(power, 7.0);
    fake_preset(&fake, "ActivePower", -7.9);
    assert_int_equal(ops->get_power(&dev, &power), 0);
    assert_close(power, -7.0);
    double repower = 0.0;
    fake_preset(&fake, "ReactivePower", 5.2);
    assert_int_equal(ops->get_repower(&dev, &repower), 0);
    assert_close(repower, 5.0);

    int mode = -1;
    fake_preset(&fake, "RunStatus", 9.0);
    assert_int_equal(ops->get_mode(&dev, &mode), 0);
    assert_int_equal(mode, 9);

    int onoff = -1;
    fake_preset(&fake, "RunStatu", 0xCF);
    assert_int_equal(ops->get_onoff(&dev, &onoff), 0);
    assert_int_equal(onoff, IEMS_VENDOR_ON);
    fake_preset(&fake, "RunStatu", 0xCE);
    assert_int_equal(ops->get_onoff(&dev, &onoff), 0);
    assert_int_equal(onoff, IEMS_VENDOR_OFF);
    // 未定义状态返回 -1，且不改写输出（v3 会先写 OFF 再返回 -1）。
    fake_preset(&fake, "RunStatu", 0.0);
    onoff = -7;
    assert_int_equal(ops->get_onoff(&dev, &onoff), -1);
    assert_int_equal(onoff, -7);

    int connect = -1;
    fake_preset(&fake, "GridStatus", 0x55);
    assert_int_equal(ops->get_connect(&dev, &connect), 0);
    assert_int_equal(connect, IEMS_VENDOR_DISCONNECT_GRID);
    fake_preset(&fake, "GridStatus", 0xA5);
    assert_int_equal(ops->get_connect(&dev, &connect), 0);
    assert_int_equal(connect, IEMS_VENDOR_CONNECT_GRID);

    // v3 把 get_mode/get_connect 误派到 get_onoff；扩展接口读同名 tag。
    fake.read_count = 0;
    assert_int_equal(ops->get_int_value(&dev, IEMS_VENDOR_FN_GET_MODE, &mode), 0);
    assert_string_equal(fake.reads[0].tag, "RunStatus");
    assert_int_equal(ops->get_int_value(&dev, IEMS_VENDOR_FN_GET_CONNECT, &connect), 0);
    assert_string_equal(fake.reads[1].tag, "GridStatus");
}

static void test_xiankong_vendor(void **state) {
    (void)state;
    fake_io fake = {0};
    iems_vendor_device dev;
    const iems_vendor_semantics *ops = bind_device(&dev, &fake, "xiankong");
    assert_int_equal(dev.compatible_index, 0);
    assert_true(ops != iems_vendor_registry_lookup("pcs_int_double", NULL));
    assert_int_equal(ops->init(&dev, NULL), 0);

    // v3 用 int 访问器写功率（xiankong_pcs.c:12-16），小数显式截断。
    assert_int_equal(ops->set_power(&dev, 10.9), 0);
    assert_int_equal(ops->set_power(&dev, -10.9), 0);
    assert_int_equal(fake.write_count, 2);
    assert_string_equal(fake.writes[0].tag, "ActPowSet");
    assert_close(fake.writes[0].value, 10.0);
    assert_close(fake.writes[1].value, -10.0);

    // v3 set_repower/set_mode 直接返回 -1，不写 tag。
    assert_int_equal(ops->set_repower(&dev, 1.0), -1);
    assert_int_equal(ops->set_mode(&dev, 1), -1);
    assert_int_equal(fake.write_count, 2);

    assert_int_equal(ops->set_onoff(&dev, IEMS_VENDOR_OFF), 0);
    assert_string_equal(fake.writes[2].tag, "OnOffSet");
    assert_close(fake.writes[2].value, 0.0);

    // 启动需要 StartReady；未就绪返回 v3 的 -2 且不写。
    fake_preset(&fake, "StartReady", 0.0);
    assert_int_equal(ops->set_onoff(&dev, IEMS_VENDOR_ON), -2);
    assert_int_equal(fake.write_count, 3);
    // v3 只判断 StartReady 是否非 0，2 也视为就绪。
    fake_preset(&fake, "StartReady", 2.0);
    assert_int_equal(ops->set_onoff(&dev, IEMS_VENDOR_ON), 0);
    assert_int_equal(fake.write_count, 4);
    assert_string_equal(fake.writes[3].tag, "OnOffSet");
    assert_close(fake.writes[3].value, 1.0);
    // 缺失 StartReady 是读失败，比 v3 缺省 0 更严格，返回 -1。
    fake.preset_count = 0;
    assert_int_equal(ops->set_onoff(&dev, IEMS_VENDOR_ON), -1);
    assert_int_equal(fake.write_count, 4);
    assert_int_equal(ops->set_onoff(&dev, IEMS_VENDOR_STANDBY), -1);
    assert_int_equal(fake.write_count, 4);
    print_writes("xiankong", &fake);

    fake.write_count = 0;
    assert_int_equal(ops->set_connect(&dev, IEMS_VENDOR_CONNECT_GRID), 0);
    assert_string_equal(fake.writes[0].tag, "WorkMode");
    assert_close(fake.writes[0].value, 1.0);
    assert_int_equal(ops->set_connect(&dev, IEMS_VENDOR_DISCONNECT_GRID), 0);
    assert_close(fake.writes[1].value, 0.0);
    assert_int_equal(ops->set_connect(&dev, 2), -1);
    assert_int_equal(fake.write_count, 2);

    // v3 的功率/模式读接口全部返回 -1。
    double power = 0.0;
    int mode = 0;
    assert_int_equal(ops->get_power(&dev, &power), -1);
    assert_int_equal(ops->get_repower(&dev, &power), -1);
    assert_int_equal(ops->get_mode(&dev, &mode), -1);

    int onoff = -1;
    fake_preset(&fake, "ModeOnOffStatus", 1.0);
    assert_int_equal(ops->get_onoff(&dev, &onoff), 0);
    assert_int_equal(onoff, IEMS_VENDOR_ON);
    fake_preset(&fake, "ModeOnOffStatus", 0.0);
    assert_int_equal(ops->get_onoff(&dev, &onoff), 0);
    assert_int_equal(onoff, IEMS_VENDOR_OFF);
    fake_preset(&fake, "ModeOnOffStatus", 3.0);
    onoff = -7;
    assert_int_equal(ops->get_onoff(&dev, &onoff), -1);
    assert_int_equal(onoff, -7);
    // 缺失 ModeOnOffStatus 是读失败，返回 -1。
    fake.preset_count = 0;
    assert_int_equal(ops->get_onoff(&dev, &onoff), -1);
    assert_int_equal(onoff, -7);

    int connect = -1;
    fake_preset(&fake, "CurWorkMode", 1.0);
    assert_int_equal(ops->get_connect(&dev, &connect), 0);
    assert_int_equal(connect, IEMS_VENDOR_DISCONNECT_GRID);
    fake_preset(&fake, "CurWorkMode", 2.0);
    assert_int_equal(ops->get_connect(&dev, &connect), 0);
    assert_int_equal(connect, IEMS_VENDOR_CONNECT_GRID);
    fake_preset(&fake, "CurWorkMode", 3.0);
    connect = -7;
    assert_int_equal(ops->get_connect(&dev, &connect), -1);
    assert_int_equal(connect, -7);
    // 缺失 CurWorkMode 是读失败，返回 -1。
    fake.preset_count = 0;
    assert_int_equal(ops->get_connect(&dev, &connect), -1);
    assert_int_equal(connect, -7);

    // v3 的 set/get WorkMode 不对称：set_connect 写 WorkMode（断开=0/并网=1），
    // get_connect 读 CurWorkMode（1=断开/2=并网）；同一个 1 在两侧含义相反。
    fake.write_count = 0;
    fake.read_count = 0;
    assert_int_equal(ops->set_connect(&dev, IEMS_VENDOR_CONNECT_GRID), 0);
    assert_string_equal(fake.writes[0].tag, "WorkMode");
    assert_close(fake.writes[0].value, 1.0);
    fake_preset(&fake, "CurWorkMode", fake.writes[0].value);
    assert_int_equal(ops->get_connect(&dev, &connect), 0);
    assert_int_equal(connect, IEMS_VENDOR_DISCONNECT_GRID);
    assert_string_equal(fake.reads[0].tag, "CurWorkMode");

    // v3 把 get_mode/get_connect 派到 get_onoff；这里按同名方法派发：
    // get_mode 未实现返回 -1 且不读 tag，get_connect 读 CurWorkMode。
    fake.read_count = 0;
    fake_preset(&fake, "ModeOnOffStatus", 1.0);
    fake_preset(&fake, "CurWorkMode", 2.0);
    assert_int_equal(ops->get_int_value(&dev, IEMS_VENDOR_FN_GET_MODE, &mode), -1);
    assert_int_equal(fake.read_count, 0);
    assert_int_equal(ops->get_int_value(&dev, IEMS_VENDOR_FN_GET_ONOFF, &onoff), 0);
    assert_string_equal(fake.reads[0].tag, "ModeOnOffStatus");
    assert_int_equal(ops->get_int_value(&dev, IEMS_VENDOR_FN_GET_CONNECT, &connect), 0);
    assert_int_equal(connect, IEMS_VENDOR_CONNECT_GRID);
    assert_string_equal(fake.reads[1].tag, "CurWorkMode");
    double value = 0.0;
    assert_int_equal(ops->get_double_value(&dev, IEMS_VENDOR_FN_GET_POWER, &value), -1);
    assert_int_equal(ops->get_double_value(&dev, "unknown", &value), -1);
}

static void test_xinai_vendor(void **state) {
    (void)state;
    fake_io fake = {0};
    iems_vendor_device dev;
    const iems_vendor_semantics *ops = bind_device(&dev, &fake, "xinai");
    assert_int_equal(dev.compatible_index, 0);
    assert_true(ops != iems_vendor_registry_lookup("pcs_onoff_01", NULL));
    assert_int_equal(ops->init(&dev, NULL), 0);

    assert_int_equal(ops->set_power(&dev, 12.5), 0);
    assert_int_equal(ops->set_repower(&dev, -4.25), 0);
    assert_int_equal(ops->set_mode(&dev, 3), 0);
    assert_int_equal(ops->set_onoff(&dev, IEMS_VENDOR_ON), 0);
    assert_int_equal(ops->set_onoff(&dev, IEMS_VENDOR_OFF), 0);
    assert_int_equal(ops->set_onoff(&dev, IEMS_VENDOR_STANDBY), -1);
    assert_int_equal(fake.write_count, 5);
    assert_string_equal(fake.writes[0].tag, "ActPowSet");
    assert_close(fake.writes[0].value, 12.5);  // v3 用 float 写，不截断
    assert_string_equal(fake.writes[1].tag, "ReactPowSet");
    assert_close(fake.writes[1].value, -4.25);
    assert_string_equal(fake.writes[2].tag, "WorkMode");
    assert_close(fake.writes[2].value, 3.0);
    assert_string_equal(fake.writes[3].tag, "StartStopCtr");
    assert_close(fake.writes[3].value, 1.0);
    assert_string_equal(fake.writes[4].tag, "StartStopCtr");
    assert_close(fake.writes[4].value, 0.0);
    print_writes("xinai", &fake);

    fake.write_count = 0;
    assert_int_equal(ops->set_connect(&dev, IEMS_VENDOR_CONNECT_GRID), 0);
    assert_string_equal(fake.writes[0].tag, "setConnect");
    assert_close(fake.writes[0].value, 1.0);
    assert_int_equal(ops->set_connect(&dev, IEMS_VENDOR_DISCONNECT_GRID), 0);
    assert_close(fake.writes[1].value, 0.0);

    // 功率/无功读回写侧 tag，按 v3 int 访问器截断。
    double power = 0.0;
    fake_preset(&fake, "ActPowSet", 7.9);
    assert_int_equal(ops->get_power(&dev, &power), 0);
    assert_close(power, 7.0);
    double repower = 0.0;
    fake_preset(&fake, "ReactPowSet", -7.9);
    assert_int_equal(ops->get_repower(&dev, &repower), 0);
    assert_close(repower, -7.0);

    int mode = -1;
    fake_preset(&fake, "WorkMode", 5.0);
    assert_int_equal(ops->get_mode(&dev, &mode), 0);
    assert_int_equal(mode, 5);

    // v3 get_onoff 读 Alarm_3（xinai_pcs.c:82-101），0/1 严格映射。
    int onoff = -1;
    fake_preset(&fake, "Alarm_3", 0.0);
    assert_int_equal(ops->get_onoff(&dev, &onoff), 0);
    assert_int_equal(onoff, IEMS_VENDOR_OFF);
    fake_preset(&fake, "Alarm_3", 1.0);
    assert_int_equal(ops->get_onoff(&dev, &onoff), 0);
    assert_int_equal(onoff, IEMS_VENDOR_ON);
    fake_preset(&fake, "Alarm_3", 2.0);
    assert_int_equal(ops->get_onoff(&dev, &onoff), -1);

    int connect = -1;
    fake_preset(&fake, "setConnect", 1.0);
    assert_int_equal(ops->get_connect(&dev, &connect), 0);
    assert_int_equal(connect, 1);

    // 扩展接口按同名方法：GET_MODE 读 WorkMode，而不是 v3 误派的 Alarm_3。
    fake.read_count = 0;
    assert_int_equal(ops->get_int_value(&dev, IEMS_VENDOR_FN_GET_MODE, &mode), 0);
    assert_string_equal(fake.reads[0].tag, "WorkMode");
    assert_int_equal(ops->get_int_value(&dev, IEMS_VENDOR_FN_GET_CONNECT, &connect), 0);
    assert_string_equal(fake.reads[1].tag, "setConnect");
}

static void test_yingbo_vendor(void **state) {
    (void)state;
    fake_io fake = {0};
    iems_vendor_device dev;
    const iems_vendor_semantics *ops = bind_device(&dev, &fake, "yingbo");
    assert_int_equal(dev.compatible_index, 0);
    assert_true(ops != iems_vendor_registry_lookup("pcs_split3", NULL));
    assert_int_equal(ops->init(&dev, NULL), 0);

    // 功率固定三相写，各值/3，不再补写总功率。
    assert_int_equal(ops->set_power(&dev, 9.0), 0);
    assert_int_equal(fake.write_count, 3);
    assert_string_equal(fake.writes[0].tag, "ASplitPhaseActPowSet");
    assert_string_equal(fake.writes[1].tag, "BSplitPhaseActPowSet");
    assert_string_equal(fake.writes[2].tag, "CSplitPhaseActPowSet");
    for (int i = 0; i < 3; i++) {
        assert_close(fake.writes[i].value, 3.0);
    }
    assert_null(find_write(&fake, "ActPowSet"));
    print_writes("yingbo (power split)", &fake);

    fake.write_count = 0;
    assert_int_equal(ops->set_repower(&dev, 6.0), 0);
    assert_int_equal(fake.write_count, 3);
    assert_string_equal(fake.writes[0].tag, "ASplitPhaseReactPowSet");
    assert_string_equal(fake.writes[1].tag, "BSplitPhaseReactPowSet");
    assert_string_equal(fake.writes[2].tag, "CSplitPhaseReactPowSet");
    for (int i = 0; i < 3; i++) {
        assert_close(fake.writes[i].value, 2.0);
    }

    fake.write_count = 0;
    assert_int_equal(ops->set_mode(&dev, 4), 0);
    assert_int_equal(ops->set_onoff(&dev, IEMS_VENDOR_ON), 0);
    assert_int_equal(ops->set_onoff(&dev, IEMS_VENDOR_OFF), 0);
    assert_int_equal(fake.write_count, 3);
    assert_string_equal(fake.writes[0].tag, "WorkMode");
    assert_close(fake.writes[0].value, 4.0);
    assert_string_equal(fake.writes[1].tag, "PCSStart");
    assert_close(fake.writes[1].value, 1.0);
    assert_string_equal(fake.writes[2].tag, "PCSStop");
    assert_close(fake.writes[2].value, 1.0);
    // v3 的 switch 没有 default 且总返回 0；这里未定义态返回 -1 且不写。
    assert_int_equal(ops->set_onoff(&dev, IEMS_VENDOR_STANDBY), -1);
    assert_int_equal(fake.write_count, 3);
    print_writes("yingbo (mode/onoff)", &fake);

    fake.write_count = 0;
    assert_int_equal(ops->set_connect(&dev, IEMS_VENDOR_CONNECT_GRID), 0);
    assert_string_equal(fake.writes[0].tag, "SetConnect");
    assert_close(fake.writes[0].value, 1.0);
    assert_int_equal(ops->set_connect(&dev, IEMS_VENDOR_DISCONNECT_GRID), 0);
    assert_close(fake.writes[1].value, 0.0);
    assert_int_equal(ops->set_connect(&dev, 2), -1);
    assert_int_equal(fake.write_count, 2);

    // 功率/无功/模式读 int 访问器并显式截断。
    double power = 0.0;
    fake_preset(&fake, "ActivePower", 7.9);
    assert_int_equal(ops->get_power(&dev, &power), 0);
    assert_close(power, 7.0);
    double repower = 0.0;
    fake_preset(&fake, "ReactivePower", -7.9);
    assert_int_equal(ops->get_repower(&dev, &repower), 0);
    assert_close(repower, -7.0);

    int mode = -1;
    fake_preset(&fake, "RunStatu", 5.0);
    assert_int_equal(ops->get_mode(&dev, &mode), 0);
    assert_int_equal(mode, 5);

    // Stoppage 非 0 判停机，0 判开机。
    int onoff = -1;
    fake_preset(&fake, "Stoppage", 1.0);
    assert_int_equal(ops->get_onoff(&dev, &onoff), 0);
    assert_int_equal(onoff, IEMS_VENDOR_OFF);
    fake_preset(&fake, "Stoppage", 0.0);
    assert_int_equal(ops->get_onoff(&dev, &onoff), 0);
    assert_int_equal(onoff, IEMS_VENDOR_ON);

    // GridOnOff 非 0 判并网，0 判离线。
    int connect = -1;
    fake_preset(&fake, "GridOnOff", 1.0);
    assert_int_equal(ops->get_connect(&dev, &connect), 0);
    assert_int_equal(connect, IEMS_VENDOR_CONNECT_GRID);
    fake_preset(&fake, "GridOnOff", 0.0);
    assert_int_equal(ops->get_connect(&dev, &connect), 0);
    assert_int_equal(connect, IEMS_VENDOR_DISCONNECT_GRID);

    // v3 把 get_mode/get_connect 误派到 get_onoff；扩展接口读同名 tag。
    fake.read_count = 0;
    assert_int_equal(ops->get_int_value(&dev, IEMS_VENDOR_FN_GET_MODE, &mode), 0);
    assert_string_equal(fake.reads[0].tag, "RunStatu");
    assert_int_equal(ops->get_int_value(&dev, IEMS_VENDOR_FN_GET_CONNECT, &connect), 0);
    assert_string_equal(fake.reads[1].tag, "GridOnOff");
}

static void test_zhongteng_vendor(void **state) {
    (void)state;
    fake_io fake = {0};
    iems_vendor_device dev;
    const iems_vendor_semantics *ops = bind_device(&dev, &fake, "zhongteng");
    assert_int_equal(dev.compatible_index, 0);
    assert_true(ops != iems_vendor_registry_lookup("pcs_standard", NULL));
    assert_int_equal(ops->init(&dev, NULL), 0);

    assert_int_equal(ops->set_power(&dev, 12.5), 0);
    assert_int_equal(ops->set_repower(&dev, -4.25), 0);
    assert_string_equal(fake.writes[0].tag, "ActPowSet");
    assert_close(fake.writes[0].value, 12.5);
    assert_string_equal(fake.writes[1].tag, "ReactPowSet");
    assert_close(fake.writes[1].value, -4.25);

    // v3 set_mode(0) 写 GridOff=1，set_mode(1) 写 GridOn=1（zhongteng.c:31-49）。
    assert_int_equal(ops->set_mode(&dev, IEMS_VENDOR_OFF), 0);
    assert_string_equal(fake.writes[2].tag, "GridOff");
    assert_close(fake.writes[2].value, 1.0);
    assert_int_equal(ops->set_mode(&dev, IEMS_VENDOR_ON), 0);
    assert_string_equal(fake.writes[3].tag, "GridOn");
    assert_close(fake.writes[3].value, 1.0);
    assert_int_equal(ops->set_mode(&dev, 2), -1);
    assert_int_equal(fake.write_count, 4);

    assert_int_equal(ops->set_onoff(&dev, IEMS_VENDOR_OFF), 0);
    assert_string_equal(fake.writes[4].tag, "StopCtr");
    assert_close(fake.writes[4].value, 1.0);
    assert_int_equal(ops->set_onoff(&dev, IEMS_VENDOR_ON), 0);
    assert_string_equal(fake.writes[5].tag, "StartCtr");
    assert_close(fake.writes[5].value, 1.0);
    assert_int_equal(ops->set_onoff(&dev, IEMS_VENDOR_STANDBY), -1);
    assert_int_equal(fake.write_count, 6);
    print_writes("zhongteng", &fake);

    fake.write_count = 0;
    assert_int_equal(ops->set_connect(&dev, IEMS_VENDOR_CONNECT_GRID), 0);
    assert_string_equal(fake.writes[0].tag, "setConnect");
    assert_close(fake.writes[0].value, 1.0);
    assert_int_equal(ops->set_connect(&dev, IEMS_VENDOR_DISCONNECT_GRID), 0);
    assert_close(fake.writes[1].value, 0.0);

    // 功率/无功读回写侧 tag，按 v3 int 访问器截断。
    double power = 0.0;
    fake_preset(&fake, "ActPowSet", 7.9);
    assert_int_equal(ops->get_power(&dev, &power), 0);
    assert_close(power, 7.0);
    double repower = 0.0;
    fake_preset(&fake, "ReactPowSet", -7.9);
    assert_int_equal(ops->get_repower(&dev, &repower), 0);
    assert_close(repower, -7.0);

    // v3 get_mode 先看 GridModelOn，再看 GridModelOff（zhongteng.c:88-103）。
    int mode = -1;
    fake_preset(&fake, "GridModelOn", 1.0);
    fake_preset(&fake, "GridModelOff", 0.0);
    assert_int_equal(ops->get_mode(&dev, &mode), 0);
    assert_int_equal(mode, IEMS_VENDOR_ON);
    fake_preset(&fake, "GridModelOn", 0.0);
    fake_preset(&fake, "GridModelOff", 1.0);
    assert_int_equal(ops->get_mode(&dev, &mode), 0);
    assert_int_equal(mode, IEMS_VENDOR_OFF);
    fake_preset(&fake, "GridModelOff", 0.0);
    assert_int_equal(ops->get_mode(&dev, &mode), -1);

    // v3 原样读 OnOff 与 setConnect，不做映射。
    int onoff = -1;
    fake_preset(&fake, "OnOff", 3.0);
    assert_int_equal(ops->get_onoff(&dev, &onoff), 0);
    assert_int_equal(onoff, 3);
    int connect = -1;
    fake_preset(&fake, "setConnect", 1.0);
    assert_int_equal(ops->get_connect(&dev, &connect), 0);
    assert_int_equal(connect, 1);

    // 扩展接口按同名方法读：GET_MODE 走 GridModelOn/Off，GET_CONNECT 走 setConnect。
    fake.read_count = 0;
    fake_preset(&fake, "GridModelOn", 1.0);
    assert_int_equal(ops->get_int_value(&dev, IEMS_VENDOR_FN_GET_MODE, &mode), 0);
    assert_string_equal(fake.reads[0].tag, "GridModelOn");
    assert_int_equal(ops->get_int_value(&dev, IEMS_VENDOR_FN_GET_CONNECT, &connect), 0);
    assert_string_equal(fake.reads[1].tag, "setConnect");
}

static void test_three_phase_split_no_double_write(void **state) {
    (void)state;
    fake_io fake = {0};
    iems_vendor_device dev;
    const iems_vendor_semantics *ops = bind_device(&dev, &fake, "pcs_split3");
    assert_int_equal(dev.compatible_index, 0);

    assert_int_equal(ops->set_power(&dev, 9.0), 0);
    assert_int_equal(fake.write_count, 1);
    assert_string_equal(fake.writes[0].tag, "ActPowSet");
    assert_close(fake.writes[0].value, 9.0);
    print_writes("pcs_split3 (single)", &fake);

    double power = 0.0;
    fake_preset(&fake, "ActivePower", 8.5);
    assert_int_equal(ops->get_power(&dev, &power), 0);
    assert_close(power, 8.5);

    fake_io split = {0};
    iems_vendor_device split_dev;
    ops = bind_device(&split_dev, &split, "pcs_split3-3p4w");
    assert_int_equal(split_dev.compatible_index, 1);

    assert_int_equal(ops->set_power(&split_dev, 9.0), 0);
    assert_int_equal(split.write_count, 3);
    assert_string_equal(split.writes[0].tag, "ASplitPhaseActPowSet");
    assert_string_equal(split.writes[1].tag, "BSplitPhaseActPowSet");
    assert_string_equal(split.writes[2].tag, "CSplitPhaseActPowSet");
    for (int i = 0; i < 3; i++) {
        assert_close(split.writes[i].value, 3.0);
    }
    // 分相写只能写三相，不能再补一笔总功率。
    assert_null(find_write(&split, "ActPowSet"));
    print_writes("pcs_split3-3p4w (power split)", &split);

    split.write_count = 0;
    assert_int_equal(ops->set_repower(&split_dev, 6.0), 0);
    assert_int_equal(split.write_count, 3);
    assert_string_equal(split.writes[0].tag, "ASplitPhaseReactPowSet");
    for (int i = 0; i < 3; i++) {
        assert_close(split.writes[i].value, 2.0);
    }
    print_writes("pcs_split3-3p4w (repower split)", &split);

    split.read_count = 0;
    fake_preset(&split, "AphaseActPower", 1.0);
    fake_preset(&split, "BphaseActPower", 2.0);
    fake_preset(&split, "CphaseActPower", 3.0);
    assert_int_equal(ops->get_power(&split_dev, &power), 0);
    assert_close(power, 3.0);
    assert_int_equal(split.read_count, 3);
    assert_string_equal(split.reads[0].tag, "AphaseActPower");
    assert_string_equal(split.reads[1].tag, "BphaseActPower");
    assert_string_equal(split.reads[2].tag, "CphaseActPower");

    fake_preset(&split, "AphaseReactivePower", 10.0);
    fake_preset(&split, "BphaseReactivePower", 20.0);
    fake_preset(&split, "CphaseReactivePower", 30.0);
    double repower = 0.0;
    assert_int_equal(ops->get_repower(&split_dev, &repower), 0);
    assert_close(repower, 30.0);

    split.write_count = 0;
    assert_int_equal(ops->set_mode(&split_dev, 4), 0);
    assert_int_equal(ops->set_onoff(&split_dev, IEMS_VENDOR_ON), 0);
    assert_int_equal(ops->set_onoff(&split_dev, IEMS_VENDOR_OFF), 0);
    assert_int_equal(ops->set_connect(&split_dev, IEMS_VENDOR_CONNECT_GRID), 0);
    assert_int_equal(ops->set_connect(&split_dev, IEMS_VENDOR_DISCONNECT_GRID), 0);
    assert_int_equal(split.write_count, 5);
    assert_string_equal(split.writes[0].tag, "WorkMode");
    assert_close(split.writes[0].value, 4.0);
    assert_string_equal(split.writes[1].tag, "PCSStart");
    assert_string_equal(split.writes[2].tag, "PCSStop");
    assert_string_equal(split.writes[3].tag, "SetConnect");
    assert_close(split.writes[3].value, 0.0);
    assert_close(split.writes[4].value, 1.0);

    int onoff = -1;
    fake_preset(&split, "Stoppage", 1.0);
    assert_int_equal(ops->get_onoff(&split_dev, &onoff), 0);
    assert_int_equal(onoff, IEMS_VENDOR_OFF);
    fake_preset(&split, "Stoppage", 0.0);
    assert_int_equal(ops->get_onoff(&split_dev, &onoff), 0);
    assert_int_equal(onoff, IEMS_VENDOR_ON);

    int mode = 0;
    fake_preset(&split, "RunStatu", 7.0);
    assert_int_equal(ops->get_mode(&split_dev, &mode), 0);
    assert_int_equal(mode, 7);

    int connect = -1;
    fake_preset(&split, "GridOnOff", 1.0);
    assert_int_equal(ops->get_connect(&split_dev, &connect), 0);
    assert_int_equal(connect, IEMS_VENDOR_DISCONNECT_GRID);
    fake_preset(&split, "GridOnOff", 0.0);
    assert_int_equal(ops->get_connect(&split_dev, &connect), 0);
    assert_int_equal(connect, IEMS_VENDOR_CONNECT_GRID);
}

static void test_onoff_conventions(void **state) {
    (void)state;
    static const struct {
        const char *compatible;
        const char *tag;
        int off_value;
        int on_value;
    } cases[] = {
        {"pcs_onoff_01", "StartStopCtr", 0x0000, 0x0001},
        {"pcs_onoff_00ff", "StartStopCtr", 0x00ff, 0xff00},
        {"pcs_onoff_5555", "OnOff", 21845, 43690},
    };

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        fake_io fake = {0};
        iems_vendor_device dev;
        const iems_vendor_semantics *ops = bind_device(&dev, &fake, cases[i].compatible);

        assert_int_equal(ops->set_onoff(&dev, IEMS_VENDOR_OFF), 0);
        assert_int_equal(fake.write_count, 1);
        assert_string_equal(fake.writes[0].tag, cases[i].tag);
        assert_close(fake.writes[0].value, (double)cases[i].off_value);
        assert_int_equal(ops->set_onoff(&dev, IEMS_VENDOR_ON), 0);
        assert_string_equal(fake.writes[1].tag, cases[i].tag);
        assert_close(fake.writes[1].value, (double)cases[i].on_value);
        print_writes(cases[i].compatible, &fake);

        int onoff = -1;
        fake_preset(&fake, cases[i].tag, (double)cases[i].off_value);
        assert_int_equal(ops->get_onoff(&dev, &onoff), 0);
        assert_int_equal(onoff, IEMS_VENDOR_OFF);
        fake_preset(&fake, cases[i].tag, (double)cases[i].on_value);
        assert_int_equal(ops->get_onoff(&dev, &onoff), 0);
        assert_int_equal(onoff, IEMS_VENDOR_ON);
        fake_preset(&fake, cases[i].tag, 12345.0);
        assert_int_equal(ops->get_onoff(&dev, &onoff), -1);

        double power = 0.0;
        assert_int_equal(ops->set_power(&dev, 1.0), -1);
        assert_int_equal(ops->set_mode(&dev, 1), -1);
        assert_int_equal(ops->get_power(&dev, &power), -1);
    }
}

static void test_int_double_conversion(void **state) {
    (void)state;
    fake_io fake = {0};
    iems_vendor_device dev;
    const iems_vendor_semantics *ops = bind_device(&dev, &fake, "pcs_int_double");

    assert_int_equal(ops->set_power(&dev, 12.9), 0);
    assert_string_equal(fake.writes[0].tag, "ActPowSet");
    assert_close(fake.writes[0].value, 12.0);
    assert_int_equal(ops->set_power(&dev, -12.9), 0);
    assert_close(fake.writes[1].value, -12.0);
    assert_int_equal(ops->set_repower(&dev, 4.7), 0);
    assert_close(fake.writes[2].value, 4.0);
    print_writes("pcs_int_double", &fake);

    double power = 0.0;
    fake_preset(&fake, "ActivePower", 7.9);
    assert_int_equal(ops->get_power(&dev, &power), 0);
    assert_close(power, 7.0);
    fake_preset(&fake, "ActivePower", -7.9);
    assert_int_equal(ops->get_power(&dev, &power), 0);
    assert_close(power, -7.0);

    double repower = 0.0;
    fake_preset(&fake, "ReactivePower", 5.2);
    assert_int_equal(ops->get_repower(&dev, &repower), 0);
    assert_close(repower, 5.0);

    int mode = 0;
    assert_int_equal(ops->set_mode(&dev, 1), -1);
    assert_int_equal(ops->get_mode(&dev, &mode), -1);
}

static void test_readonly_sum_and_unsupported(void **state) {
    (void)state;
    fake_io fake = {0};
    iems_vendor_device dev;
    const iems_vendor_semantics *ops = bind_device(&dev, &fake, "mppt_sum4");

    assert_int_equal(ops->set_power(&dev, 5.0), -1);
    assert_int_equal(ops->set_repower(&dev, 5.0), -1);
    assert_int_equal(ops->set_mode(&dev, 1), -1);
    assert_int_equal(ops->set_onoff(&dev, IEMS_VENDOR_ON), -1);
    assert_int_equal(ops->set_connect(&dev, IEMS_VENDOR_CONNECT_GRID), -1);
    assert_int_equal(fake.write_count, 0);

    fake_preset(&fake, "InputPower1", 1.0);
    fake_preset(&fake, "InputPower2", 2.0);
    fake_preset(&fake, "InputPower3", 3.0);
    fake_preset(&fake, "InputPower4", 4.0);
    double power = 0.0;
    assert_int_equal(ops->get_power(&dev, &power), 0);
    assert_close(power, 10.0);
    assert_int_equal(fake.read_count, 4);
    assert_string_equal(fake.reads[0].tag, "InputPower1");
    assert_string_equal(fake.reads[3].tag, "InputPower4");

    // 任一路读取失败都要上报，不能把缺数据当 0。
    fake.preset_count = 0;
    fake_preset(&fake, "InputPower1", 1.0);
    fake_preset(&fake, "InputPower2", 2.0);
    fake_preset(&fake, "InputPower3", 3.0);
    assert_int_equal(ops->get_power(&dev, &power), -1);

    fake_preset(&fake, "InputPower4", 4.0);
    int mode = 0;
    double repower = 0.0;
    assert_int_equal(ops->get_repower(&dev, &repower), -1);
    assert_int_equal(ops->get_mode(&dev, &mode), -1);
    assert_int_equal(ops->get_int_value(&dev, IEMS_VENDOR_FN_GET_ONOFF, &mode), -1);
    assert_int_equal(ops->get_double_value(&dev, IEMS_VENDOR_FN_GET_POWER, &power), 0);
    assert_close(power, 10.0);
}

static void check_forward_vendor(const char *compatible, const char *other_compatible) {
    fake_io fake = {0};
    iems_vendor_device dev;
    const iems_vendor_semantics *ops = bind_device(&dev, &fake, compatible);
    assert_int_equal(dev.compatible_index, 0);
    assert_true(ops != iems_vendor_registry_lookup(other_compatible, NULL));
    assert_int_equal(ops->init(&dev, NULL), 0);

    assert_int_equal(ops->set_power(&dev, 12.5), 0);
    assert_int_equal(fake.write_count, 1);
    assert_string_equal(fake.writes[0].tag, "set_power");
    assert_close(fake.writes[0].value, 12.5);
    print_writes(compatible, &fake);

    // v3 只把模板 function get_power 转发给端口，适配器内没有 tag 常量。
    double power = 0.0;
    fake_preset(&fake, "get_power", 7.5);
    assert_int_equal(ops->get_power(&dev, &power), 0);
    assert_close(power, 7.5);
    assert_string_equal(fake.reads[fake.read_count - 1].tag, "get_power");

    int int_value = 0;
    double repower = 0.0;
    assert_int_equal(ops->set_repower(&dev, 1.0), -1);
    assert_int_equal(ops->set_mode(&dev, 1), -1);
    assert_int_equal(ops->set_onoff(&dev, IEMS_VENDOR_ON), -1);
    assert_int_equal(ops->set_connect(&dev, IEMS_VENDOR_CONNECT_GRID), -1);
    assert_int_equal(ops->get_repower(&dev, &repower), -1);
    assert_int_equal(ops->get_mode(&dev, &int_value), -1);
    assert_int_equal(ops->get_onoff(&dev, &int_value), -1);
    assert_int_equal(ops->get_connect(&dev, &int_value), -1);
    assert_int_equal(fake.write_count, 1);

    assert_int_equal(ops->set_value(&dev, IEMS_VENDOR_FN_SET_POWER, 3.0), 0);
    assert_int_equal(fake.write_count, 2);
    assert_string_equal(fake.writes[1].tag, "set_power");
    assert_close(fake.writes[1].value, 3.0);
    assert_int_equal(ops->get_double_value(&dev, IEMS_VENDOR_FN_GET_POWER, &power), 0);
    assert_close(power, 7.5);
    assert_int_equal(ops->get_int_value(&dev, IEMS_VENDOR_FN_GET_ONOFF, &int_value), -1);

    fake.preset_count = 0;
    assert_int_equal(ops->get_power(&dev, &power), -1);
}

static void test_absolute_pv_vendor(void **state) {
    (void)state;
    check_forward_vendor("absolute_PV", "absolute");
}

static void test_absolute_mppt_vendor(void **state) {
    (void)state;
    check_forward_vendor("absolute", "absolute_PV");
}

static void check_power_read_vendor(const char *compatible, const char *other_compatible) {
    fake_io fake = {0};
    iems_vendor_device dev;
    const iems_vendor_semantics *ops = bind_device(&dev, &fake, compatible);
    assert_int_equal(dev.compatible_index, 0);
    assert_true(ops != iems_vendor_registry_lookup(other_compatible, NULL));
    assert_int_equal(ops->init(&dev, NULL), 0);

    assert_int_equal(ops->set_power(&dev, 1.0), -1);
    assert_int_equal(fake.write_count, 0);

    double power = 0.0;
    fake_preset(&fake, "get_power", 5.5);
    assert_int_equal(ops->get_power(&dev, &power), 0);
    assert_close(power, 5.5);
    assert_string_equal(fake.reads[fake.read_count - 1].tag, "get_power");
    print_writes(compatible, &fake);

    fake.preset_count = 0;
    assert_int_equal(ops->get_power(&dev, &power), -1);
    assert_int_equal(ops->get_repower(&dev, &power), -1);
    assert_int_equal(ops->get_double_value(&dev, IEMS_VENDOR_FN_GET_POWER, &power), -1);

    int int_value = 0;
    assert_int_equal(ops->set_value(&dev, IEMS_VENDOR_FN_SET_POWER, 1.0), -1);
    assert_int_equal(ops->get_int_value(&dev, IEMS_VENDOR_FN_GET_ONOFF, &int_value), -1);
}

static void test_bms_vendor(void **state) {
    (void)state;
    check_power_read_vendor("bms", "grid");
}

static void test_grid_vendor(void **state) {
    (void)state;
    check_power_read_vendor("grid", "bms");
}

static void test_percentage_relative_vendor(void **state) {
    (void)state;
    check_power_read_vendor("percentage_relative", "percentage_abs");
}

static void test_percentage_abs_vendor(void **state) {
    (void)state;
    fake_io fake = {0};
    iems_vendor_device dev;
    const iems_vendor_semantics *ops = bind_device(&dev, &fake, "percentage_abs");
    assert_int_equal(dev.compatible_index, 0);
    assert_true(ops != iems_vendor_registry_lookup("percentage_relative", NULL));

    // 期望/当前*100：5/10*100=50，2/8*100=25。
    fake_preset(&fake, "get_power", 10.0);
    assert_int_equal(ops->set_power(&dev, 5.0), 0);
    assert_int_equal(fake.write_count, 1);
    assert_string_equal(fake.writes[0].tag, "OutPowerSet");
    assert_close(fake.writes[0].value, 50.0);
    print_writes("percentage_abs (5/10*100)", &fake);

    fake_preset(&fake, "get_power", 8.0);
    assert_int_equal(ops->set_power(&dev, 2.0), 0);
    assert_close(fake.writes[1].value, 25.0);

    // 当前功率读失败或为 0：上报 -1 且不写（v3 为 0 时除零）。
    fake.preset_count = 0;
    assert_int_equal(ops->set_power(&dev, 2.0), -1);
    fake_preset(&fake, "get_power", 0.0);
    assert_int_equal(ops->set_power(&dev, 2.0), -1);
    assert_int_equal(fake.write_count, 2);

    double power = 0.0;
    fake_preset(&fake, "get_power", 7.5);
    assert_int_equal(ops->get_power(&dev, &power), 0);
    assert_close(power, 7.5);
    fake.preset_count = 0;
    assert_int_equal(ops->get_power(&dev, &power), -1);

    int int_value = 0;
    assert_int_equal(ops->set_repower(&dev, 1.0), -1);
    assert_int_equal(ops->get_mode(&dev, &int_value), -1);
}

static void test_mppt_ytsn_vendor(void **state) {
    (void)state;
    fake_io fake = {0};
    iems_vendor_device dev;
    const iems_vendor_semantics *ops = bind_device(&dev, &fake, "mppt_ytsn");
    assert_int_equal(dev.compatible_index, 0);
    assert_true(ops != iems_vendor_registry_lookup("mppt_sum4", NULL));

    assert_int_equal(ops->set_power(&dev, 5.0), -1);
    assert_int_equal(fake.write_count, 0);

    fake_preset(&fake, "InputPower1", 1.0);
    fake_preset(&fake, "InputPower2", 2.0);
    fake_preset(&fake, "InputPower3", 3.0);
    fake_preset(&fake, "InputPower4", 4.0);
    double power = 0.0;
    assert_int_equal(ops->get_power(&dev, &power), 0);
    assert_close(power, 10.0);
    assert_int_equal(fake.read_count, 4);
    assert_string_equal(fake.reads[0].tag, "InputPower1");
    assert_string_equal(fake.reads[3].tag, "InputPower4");
    print_message("mppt_ytsn get_power: 1+2+3+4 -> %g\n", power);

    // 任一路缺失上报 -1，不能把缺数据当 0。
    fake.preset_count = 0;
    fake_preset(&fake, "InputPower1", 1.0);
    fake_preset(&fake, "InputPower2", 2.0);
    fake_preset(&fake, "InputPower3", 3.0);
    assert_int_equal(ops->get_power(&dev, &power), -1);

    double repower = 0.0;
    assert_int_equal(ops->get_repower(&dev, &repower), -1);
    assert_int_equal(ops->set_value(&dev, IEMS_VENDOR_FN_SET_POWER, 1.0), -1);
}

static void test_livotek_hybrid_vendor(void **state) {
    (void)state;
    fake_io fake = {0};
    iems_vendor_device dev;
    const iems_vendor_semantics *ops = bind_device(&dev, &fake, "livotek_hybrid");
    assert_int_equal(dev.compatible_index, 0);
    assert_int_equal(ops->init(&dev, NULL), 0);

    // 模式与无效值已就绪时只写目标功率，不产生额外写。
    fake_preset(&fake, "EnergyManageMode", 3.0);
    fake_preset(&fake, "ExportPowerMode", 0xA0);
    fake_preset(&fake, "PVPowerLimit", 21474836.475);
    fake_preset(&fake, "BatPower", 21474836.475);
    assert_int_equal(ops->set_power(&dev, 11.5), 0);
    assert_int_equal(fake.write_count, 1);
    assert_string_equal(fake.writes[0].tag, "InverterPower");
    assert_close(fake.writes[0].value, 11.5);
    print_writes("livotek_hybrid (prelude ready)", &fake);

    // 缺 tag 时 v3 的 dev_get 返回 0，因而强制写模式与无效值，再写总功率。
    fake_io empty = {0};
    iems_vendor_device empty_dev;
    ops = bind_device(&empty_dev, &empty, "livotek_hybrid");
    assert_int_equal(ops->set_power(&empty_dev, 5.0), 0);
    assert_int_equal(empty.write_count, 5);
    assert_string_equal(empty.writes[0].tag, "EnergyManageMode");
    assert_close(empty.writes[0].value, 3.0);
    assert_string_equal(empty.writes[1].tag, "ExportPowerMode");
    assert_close(empty.writes[1].value, 0xA0);
    assert_string_equal(empty.writes[2].tag, "PVPowerLimit");
    assert_close(empty.writes[2].value, 21474836.475);
    assert_string_equal(empty.writes[3].tag, "BatPower");
    assert_close(empty.writes[3].value, 21474836.475);
    assert_string_equal(empty.writes[4].tag, "InverterPower");
    assert_close(empty.writes[4].value, 5.0);
    print_writes("livotek_hybrid (set_power from empty tags)", &empty);

    // 分相总功率：ExportPowerMode=0xA1，A/B/C 各 power/3。
    empty.write_count = 0;
    assert_int_equal(ops->set_value(&empty_dev, "set_phase_power", 9.0), 0);
    assert_int_equal(empty.write_count, 7);
    assert_string_equal(empty.writes[1].tag, "ExportPowerMode");
    assert_close(empty.writes[1].value, 0xA1);
    assert_string_equal(empty.writes[4].tag, "L1Power");
    assert_close(empty.writes[4].value, 3.0);
    assert_string_equal(empty.writes[5].tag, "L2Power");
    assert_close(empty.writes[5].value, 3.0);
    assert_string_equal(empty.writes[6].tag, "L3Power");
    assert_close(empty.writes[6].value, 3.0);
    print_writes("livotek_hybrid (set_phase_power)", &empty);

    // 单相写只动本相。
    empty.write_count = 0;
    assert_int_equal(ops->set_value(&empty_dev, "func_set_b_phase_power", 4.5), 0);
    assert_int_equal(empty.write_count, 5);
    assert_string_equal(empty.writes[4].tag, "L2Power");
    assert_close(empty.writes[4].value, 4.5);

    // PV/电池命令不切 ExportPowerMode，只把其它侧置无效。
    empty.write_count = 0;
    assert_int_equal(ops->set_value(&empty_dev, "set_pv_power", 6.0), 0);
    assert_int_equal(empty.write_count, 4);
    assert_string_equal(empty.writes[0].tag, "EnergyManageMode");
    assert_string_equal(empty.writes[1].tag, "InverterPower");
    assert_close(empty.writes[1].value, 21474836.475);
    assert_string_equal(empty.writes[2].tag, "BatPower");
    assert_string_equal(empty.writes[3].tag, "PVPowerLimit");
    assert_close(empty.writes[3].value, 6.0);
    print_writes("livotek_hybrid (set_pv_power)", &empty);

    empty.write_count = 0;
    assert_int_equal(ops->set_value(&empty_dev, "set_bat_power", 7.0), 0);
    assert_int_equal(empty.write_count, 4);
    assert_string_equal(empty.writes[1].tag, "InverterPower");
    assert_string_equal(empty.writes[2].tag, "PVPowerLimit");
    assert_string_equal(empty.writes[3].tag, "BatPower");
    assert_close(empty.writes[3].value, 7.0);

    // 柴发 function 只在值不同时写。
    empty.write_count = 0;
    assert_int_equal(ops->set_value(&empty_dev, "set_gen_on_off_func", 0x55), 0);
    assert_int_equal(empty.write_count, 1);
    assert_string_equal(empty.writes[0].tag, "GenFunction");
    assert_close(empty.writes[0].value, 0x55);
    fake_preset(&empty, "GenFunction", 0x55);
    empty.write_count = 0;
    assert_int_equal(ops->set_value(&empty_dev, "set_gen_on_off_func", 0x55), 0);
    assert_int_equal(empty.write_count, 0);

    // v3 对 OFF 写 206、ON 写 207（常量名与实际方向相反）。
    empty.write_count = 0;
    assert_int_equal(ops->set_onoff(&empty_dev, IEMS_VENDOR_OFF), 0);
    assert_string_equal(empty.writes[0].tag, "StartStopCtr");
    assert_close(empty.writes[0].value, 206.0);
    assert_int_equal(ops->set_onoff(&empty_dev, IEMS_VENDOR_ON), 0);
    assert_close(empty.writes[1].value, 207.0);
    assert_int_equal(ops->set_onoff(&empty_dev, IEMS_VENDOR_STANDBY), -1);
    assert_int_equal(empty.write_count, 2);
    print_writes("livotek_hybrid (on/off)", &empty);

    int onoff = -1;
    fake_preset(&empty, "RunStatu", 0xCF);
    assert_int_equal(ops->get_onoff(&empty_dev, &onoff), 0);
    assert_int_equal(onoff, IEMS_VENDOR_ON);
    fake_preset(&empty, "RunStatu", 0xCE);
    assert_int_equal(ops->get_onoff(&empty_dev, &onoff), 0);
    assert_int_equal(onoff, IEMS_VENDOR_OFF);
    fake_preset(&empty, "RunStatu", 1.0);
    assert_int_equal(ops->get_onoff(&empty_dev, &onoff), -1);

    empty.preset_count = 0;
    empty.read_count = 0;
    assert_int_equal(ops->get_onoff(&empty_dev, &onoff), -1);

    int connect = -1;
    fake_preset(&empty, "GridStatus", 0x55);
    assert_int_equal(ops->get_connect(&empty_dev, &connect), 0);
    assert_int_equal(connect, IEMS_VENDOR_DISCONNECT_GRID);
    fake_preset(&empty, "GridStatus", 0xA5);
    assert_int_equal(ops->get_connect(&empty_dev, &connect), 0);
    assert_int_equal(connect, IEMS_VENDOR_CONNECT_GRID);

    double power = 0.0;
    double repower = 0.0;
    fake_preset(&empty, "ActivePower", 7.5);
    assert_int_equal(ops->get_power(&empty_dev, &power), 0);
    assert_close(power, 7.5);
    fake_preset(&empty, "ReactivePower", -1.5);
    assert_int_equal(ops->get_repower(&empty_dev, &repower), 0);
    assert_close(repower, -1.5);

    // v3 空实现的写方法与不写输出的 get_mode：返回 -1。
    int int_value = 0;
    assert_int_equal(ops->set_repower(&empty_dev, 1.0), -1);
    assert_int_equal(ops->set_mode(&empty_dev, 1), -1);
    assert_int_equal(ops->set_connect(&empty_dev, IEMS_VENDOR_CONNECT_GRID), -1);
    assert_int_equal(ops->get_mode(&empty_dev, &int_value), -1);
    assert_int_equal(ops->get_int_value(&empty_dev, IEMS_VENDOR_FN_GET_MODE, &int_value), -1);

    // 扩展读：function -> tag 与 int 截断逐条钉住。
    static const struct {
        const char *function;
        const char *tag;
        double preset;
        double expected;
    } reads[] = {
        {"get_pv_power", "TotalDCPower", 3.5, 3.5},
        {"get_bat_power", "BatteryPowera", 4.9, 4.0},
        {"get_bat_soc", "BatteryPackSOC", 88.9, 88.0},
        {"get_total_input_energy", "TotalACInputEnergy", 1.25, 1.25},
        {"get_total_output_energy", "TotalACOutputEnergy", 2.5, 2.5},
        {"get_pv_cumulative_energy", "TotalGenEnergy", 3.75, 3.75},
        {"get_bat_charge_energy", "TotalChargeEnergy", 4.25, 4.25},
        {"get_bat_discharge_energy", "TotalDischargeEnergy", 5.25, 5.25},
        {"get_bat_voltage", "BatteryVoltage", 51.2, 51.2},
        {"get_bat_current", "BatteryCurrent", -12.4, -12.4},
        {"func_get_a_phase_power", "AphaseActPower", 1.9, 1.0},
        {"func_get_b_phase_power", "BphaseActPower", 2.9, 2.0},
        {"func_get_c_phase_power", "CphaseActPower", 3.9, 3.0},
        {"get_inveter_discharge_max_actual", "RatedActivePwr", 30.0, 30.0},
        {"get_inveter_charge_max_actual", "RatedActivePwr", 30.0, 30.0},
        {"get_grid_r_volt", "GridRVoltage", 230.5, 230.5},
        {"get_grid_s_volt", "GridSVoltage", 231.5, 231.5},
        {"get_grid_t_volt", "GridTVoltage", 232.5, 232.5},
    };
    for (size_t i = 0; i < sizeof(reads) / sizeof(reads[0]); i++) {
        empty.read_count = 0;
        fake_preset(&empty, reads[i].tag, reads[i].preset);
        double value = 0.0;
        assert_int_equal(ops->get_double_value(&empty_dev, reads[i].function, &value), 0);
        assert_string_equal(empty.reads[0].tag, reads[i].tag);
        assert_close(value, reads[i].expected);
    }

    int is_host = -1;
    fake_preset(&empty, "MasterSlaveSet", 0xA0);
    assert_int_equal(ops->get_int_value(&empty_dev, "get_is_host", &is_host), 0);
    assert_int_equal(is_host, 1);
    fake_preset(&empty, "MasterSlaveSet", 0x11);
    assert_int_equal(ops->get_int_value(&empty_dev, "get_is_host", &is_host), 0);
    assert_int_equal(is_host, 0);

    assert_int_equal(ops->set_value(&empty_dev, "no_such_function", 1.0), -1);
    assert_int_equal(ops->get_int_value(&empty_dev, "no_such_function", &int_value), -1);
    assert_int_equal(ops->get_double_value(&empty_dev, "no_such_function", &power), -1);
}

static void test_enjoy_vendor(void **state) {
    (void)state;
    fake_io fake = {0};
    iems_vendor_device dev;
    const iems_vendor_semantics *ops = bind_device(&dev, &fake, "enjoy");
    assert_int_equal(dev.compatible_index, 0);
    assert_true(ops != iems_vendor_registry_lookup("coepo", NULL));
    assert_int_equal(ops->init(&dev, NULL), 0);

    assert_int_equal(ops->set_power(&dev, 12.5), 0);
    assert_int_equal(ops->set_repower(&dev, -4.25), 0);
    assert_int_equal(ops->set_mode(&dev, 3), 0);
    assert_int_equal(ops->set_connect(&dev, IEMS_VENDOR_CONNECT_GRID), 0);
    assert_int_equal(ops->set_connect(&dev, IEMS_VENDOR_DISCONNECT_GRID), 0);
    assert_int_equal(fake.write_count, 5);
    assert_string_equal(fake.writes[0].tag, "ActPowSet");
    assert_close(fake.writes[0].value, 12.5);
    assert_string_equal(fake.writes[1].tag, "ReactPowSet");
    assert_close(fake.writes[1].value, -4.25);
    assert_string_equal(fake.writes[2].tag, "WorkMode");
    assert_close(fake.writes[2].value, 3.0);
    assert_string_equal(fake.writes[3].tag, "setConnect");
    assert_close(fake.writes[3].value, 0.0);
    assert_string_equal(fake.writes[4].tag, "setConnect");
    assert_close(fake.writes[4].value, 1.0);
    print_writes("enjoy", &fake);

    // 开关机联锁：离线或不满足并网条件不写，在线且并网写 0/1。
    fake.write_count = 0;
    assert_int_equal(ops->set_onoff(&dev, IEMS_VENDOR_ON), -1);
    assert_int_equal(fake.write_count, 0);
    fake_preset(&fake, "Online", 1.0);
    fake_preset(&fake, "SetGridModel", 1.0);
    assert_int_equal(ops->set_onoff(&dev, IEMS_VENDOR_ON), -1);
    assert_int_equal(fake.write_count, 0);
    fake_preset(&fake, "SetGridModel", 0.0);
    assert_int_equal(ops->set_onoff(&dev, IEMS_VENDOR_ON), 0);
    assert_int_equal(ops->set_onoff(&dev, IEMS_VENDOR_OFF), 0);
    assert_int_equal(ops->set_onoff(&dev, IEMS_VENDOR_STANDBY), -1);
    assert_int_equal(fake.write_count, 2);
    assert_string_equal(fake.writes[0].tag, "StartStopCtr");
    assert_close(fake.writes[0].value, 1.0);
    assert_string_equal(fake.writes[1].tag, "StartStopCtr");
    assert_close(fake.writes[1].value, 0.0);
    print_writes("enjoy (onoff interlock)", &fake);

    // v3 用 int 访问器读回功率，显式截断。
    double power = 0.0;
    fake_preset(&fake, "ActPowSet", 7.9);
    assert_int_equal(ops->get_power(&dev, &power), 0);
    assert_close(power, 7.0);
    double repower = 0.0;
    fake_preset(&fake, "ReactPowSet", -7.9);
    assert_int_equal(ops->get_repower(&dev, &repower), 0);
    assert_close(repower, -7.0);

    int mode = -1;
    fake_preset(&fake, "WorkMode", 5.0);
    assert_int_equal(ops->get_mode(&dev, &mode), 0);
    assert_int_equal(mode, 5);

    int onoff = -1;
    fake_preset(&fake, "StartStopCtr", 1.0);
    assert_int_equal(ops->get_onoff(&dev, &onoff), 0);
    assert_int_equal(onoff, IEMS_VENDOR_ON);
    fake_preset(&fake, "StartStopCtr", 0.0);
    assert_int_equal(ops->get_onoff(&dev, &onoff), 0);
    assert_int_equal(onoff, IEMS_VENDOR_OFF);
    fake_preset(&fake, "StartStopCtr", 2.0);
    assert_int_equal(ops->get_onoff(&dev, &onoff), -1);

    int connect = -1;
    fake_preset(&fake, "setConnect", 1.0);
    assert_int_equal(ops->get_connect(&dev, &connect), 0);
    assert_int_equal(connect, 1);

    // v3 把 get_mode/get_connect 误派到 get_onoff；扩展接口必须读同名 tag。
    fake.read_count = 0;
    assert_int_equal(ops->get_int_value(&dev, IEMS_VENDOR_FN_GET_MODE, &mode), 0);
    assert_string_equal(fake.reads[0].tag, "WorkMode");
    assert_int_equal(ops->get_int_value(&dev, IEMS_VENDOR_FN_GET_CONNECT, &connect), 0);
    assert_string_equal(fake.reads[1].tag, "setConnect");
}

static void test_feilton_vendor(void **state) {
    (void)state;
    fake_io fake = {0};
    iems_vendor_device dev;
    const iems_vendor_semantics *ops = bind_device(&dev, &fake, "feilton");
    assert_int_equal(dev.compatible_index, 0);
    assert_int_equal(ops->init(&dev, NULL), 0);

    // SetPwrMode 0 单点给定；512 三相各 1/3；其他值或读失败拒绝。
    fake_preset(&fake, "SetPwrMode", 0.0);
    assert_int_equal(ops->set_power(&dev, 12.5), 0);
    assert_int_equal(fake.write_count, 1);
    assert_string_equal(fake.writes[0].tag, "WrtActPowSet");
    assert_close(fake.writes[0].value, 12.5);
    print_writes("feilton (single)", &fake);

    fake.write_count = 0;
    fake_preset(&fake, "SetPwrMode", 512.0);
    assert_int_equal(ops->set_power(&dev, 12.5), 0);
    assert_int_equal(fake.write_count, 3);
    assert_string_equal(fake.writes[0].tag, "WrtActPowSetA");
    assert_string_equal(fake.writes[1].tag, "WrtActPowSetB");
    assert_string_equal(fake.writes[2].tag, "WrtActPowSetC");
    for (int i = 0; i < 3; i++) {
        assert_close(fake.writes[i].value, 12.5 / 3.0);
    }
    print_writes("feilton (split)", &fake);

    fake.write_count = 0;
    fake_preset(&fake, "SetPwrMode", 7.0);
    assert_int_equal(ops->set_power(&dev, 12.5), -1);
    assert_int_equal(fake.write_count, 0);
    fake.preset_count = 0;
    assert_int_equal(ops->set_power(&dev, 12.5), -1);
    assert_int_equal(fake.write_count, 0);

    // 停机写 Stop，启动需 StartReady == 1 才写 Start；未就绪返回 v3 的 -2。
    fake.write_count = 0;
    assert_int_equal(ops->set_onoff(&dev, IEMS_VENDOR_OFF), 0);
    assert_string_equal(fake.writes[0].tag, "Stop");
    assert_close(fake.writes[0].value, 5461072.0);
    fake_preset(&fake, "StartReady", 0.0);
    assert_int_equal(ops->set_onoff(&dev, IEMS_VENDOR_ON), -2);
    assert_int_equal(fake.write_count, 1);
    // v3 只认 StartReady == 1；其他可读值同样按未就绪返回 -2。
    fake_preset(&fake, "StartReady", 2.0);
    assert_int_equal(ops->set_onoff(&dev, IEMS_VENDOR_ON), -2);
    assert_int_equal(fake.write_count, 1);
    // 缺失 StartReady 是读失败，比 v3 缺省 0 更严格，返回 -1 而不是 -2。
    fake.preset_count = 0;
    assert_int_equal(ops->set_onoff(&dev, IEMS_VENDOR_ON), -1);
    assert_int_equal(fake.write_count, 1);
    fake_preset(&fake, "StartReady", 1.0);
    assert_int_equal(ops->set_onoff(&dev, IEMS_VENDOR_ON), 0);
    assert_int_equal(ops->set_onoff(&dev, IEMS_VENDOR_STANDBY), -1);
    assert_int_equal(fake.write_count, 2);
    assert_string_equal(fake.writes[1].tag, "Start");
    assert_close(fake.writes[1].value, 5395790.0);
    print_writes("feilton (onoff)", &fake);

    // 写失败向上传播为 -1（v3 忽略写结果恒返回 0）。
    fake.fail_write = -1;
    fake_preset(&fake, "SetPwrMode", 0.0);
    assert_int_equal(ops->set_power(&dev, 1.0), -1);
    fake_preset(&fake, "SetPwrMode", 512.0);
    assert_int_equal(ops->set_power(&dev, 1.0), -1);
    assert_int_equal(fake.write_count, 2);
    fake.fail_write = 0;

    // v3 StopState 0 表示运行、1 表示停机。
    int onoff = -1;
    fake_preset(&fake, "StopState", 0.0);
    assert_int_equal(ops->get_onoff(&dev, &onoff), 0);
    assert_int_equal(onoff, IEMS_VENDOR_ON);
    fake_preset(&fake, "StopState", 1.0);
    assert_int_equal(ops->get_onoff(&dev, &onoff), 0);
    assert_int_equal(onoff, IEMS_VENDOR_OFF);
    fake_preset(&fake, "StopState", 2.0);
    assert_int_equal(ops->get_onoff(&dev, &onoff), -1);
    // 缺失/坏 tag 是读失败，返回 -1。
    fake.preset_count = 0;
    onoff = -7;
    assert_int_equal(ops->get_onoff(&dev, &onoff), -1);
    assert_int_equal(onoff, -7);

    // v3 其余方法未实现：返回 -1 且不碰 I/O。
    int writes_before = fake.write_count;
    int reads_before = fake.read_count;
    double power = 0.0;
    double repower = 0.0;
    int mode = 0;
    int connect = 0;
    assert_int_equal(ops->set_repower(&dev, 1.0), -1);
    assert_int_equal(ops->set_mode(&dev, 1), -1);
    assert_int_equal(ops->set_connect(&dev, IEMS_VENDOR_CONNECT_GRID), -1);
    assert_int_equal(ops->get_power(&dev, &power), -1);
    assert_int_equal(ops->get_repower(&dev, &repower), -1);
    assert_int_equal(ops->get_mode(&dev, &mode), -1);
    assert_int_equal(ops->get_connect(&dev, &connect), -1);
    assert_int_equal(fake.write_count, writes_before);
    assert_int_equal(fake.read_count, reads_before);

    // 扩展接口按函数名派发，get_mode 不再落到 get_onoff（v3 误派）。
    assert_int_equal(ops->get_int_value(&dev, IEMS_VENDOR_FN_GET_MODE, &mode), -1);
    assert_int_equal(fake.read_count, reads_before);
    // 支持的方法经扩展接口同样写同一寄存器；未知名返回 -1 且不碰 I/O。
    fake.write_count = 0;
    fake.read_count = 0;
    fake_preset(&fake, "StopState", 0.0);
    assert_int_equal(ops->set_value(&dev, IEMS_VENDOR_FN_SET_ONOFF, IEMS_VENDOR_OFF), 0);
    assert_string_equal(fake.writes[0].tag, "Stop");
    assert_close(fake.writes[0].value, 5461072.0);
    assert_int_equal(ops->set_value(&dev, "unknown", 0.0), -1);
    assert_int_equal(fake.write_count, 1);
    assert_int_equal(ops->get_int_value(&dev, IEMS_VENDOR_FN_GET_ONOFF, &onoff), 0);
    assert_int_equal(onoff, IEMS_VENDOR_ON);
    assert_string_equal(fake.reads[0].tag, "StopState");
    assert_int_equal(ops->get_int_value(&dev, IEMS_VENDOR_FN_GET_CONNECT, &connect), -1);
    assert_int_equal(fake.read_count, 1);
    double value = 0.0;
    assert_int_equal(ops->get_double_value(&dev, IEMS_VENDOR_FN_GET_POWER, &value), -1);
    assert_int_equal(ops->get_double_value(&dev, "unknown", &value), -1);
}

static void test_huazhi_vendor(void **state) {
    (void)state;
    fake_io fake = {0};
    iems_vendor_device dev;
    const iems_vendor_semantics *ops = bind_device(&dev, &fake, "RTC");
    assert_int_equal(dev.compatible_index, 0);
    assert_int_equal(ops->init(&dev, NULL), 0);

    assert_int_equal(ops->set_power(&dev, 12.5), 0);
    assert_int_equal(ops->set_repower(&dev, -4.25), 0);
    assert_int_equal(ops->set_mode(&dev, 3), 0);
    assert_int_equal(ops->set_connect(&dev, IEMS_VENDOR_CONNECT_GRID), 0);
    assert_int_equal(ops->set_connect(&dev, IEMS_VENDOR_DISCONNECT_GRID), 0);
    assert_int_equal(fake.write_count, 5);
    assert_string_equal(fake.writes[0].tag, "ActPowSet");
    assert_close(fake.writes[0].value, 12.5);
    assert_string_equal(fake.writes[1].tag, "ReactPowSet");
    assert_close(fake.writes[1].value, -4.25);
    assert_string_equal(fake.writes[2].tag, "WorkMode");
    assert_close(fake.writes[2].value, 3.0);
    assert_string_equal(fake.writes[3].tag, "setConnect");
    assert_close(fake.writes[3].value, 1.0);
    assert_string_equal(fake.writes[4].tag, "setConnect");
    assert_close(fake.writes[4].value, 0.0);
    print_writes("huazhi (RTC)", &fake);

    // 写 OnOff（43690 开 / 21845 停），读 StartStopCtr（0 停 / 2..6 开）。
    fake.write_count = 0;
    assert_int_equal(ops->set_onoff(&dev, IEMS_VENDOR_ON), 0);
    assert_int_equal(ops->set_onoff(&dev, IEMS_VENDOR_OFF), 0);
    assert_int_equal(ops->set_onoff(&dev, IEMS_VENDOR_STANDBY), -1);
    assert_int_equal(fake.write_count, 2);
    assert_string_equal(fake.writes[0].tag, "OnOff");
    assert_close(fake.writes[0].value, 43690.0);
    assert_string_equal(fake.writes[1].tag, "OnOff");
    assert_close(fake.writes[1].value, 21845.0);
    print_writes("huazhi (onoff)", &fake);

    int onoff = -1;
    fake_preset(&fake, "StartStopCtr", 0.0);
    assert_int_equal(ops->get_onoff(&dev, &onoff), 0);
    assert_int_equal(onoff, IEMS_VENDOR_OFF);
    for (int run_state = 2; run_state <= 6; run_state++) {
        fake_preset(&fake, "StartStopCtr", (double)run_state);
        assert_int_equal(ops->get_onoff(&dev, &onoff), 0);
        assert_int_equal(onoff, IEMS_VENDOR_ON);
    }
    fake_preset(&fake, "StartStopCtr", 1.0);
    assert_int_equal(ops->get_onoff(&dev, &onoff), -1);

    // v3 用 int 访问器读回功率，显式截断。
    double power = 0.0;
    fake_preset(&fake, "ActPowSet", 7.9);
    assert_int_equal(ops->get_power(&dev, &power), 0);
    assert_close(power, 7.0);
    double repower = 0.0;
    fake_preset(&fake, "ReactPowSet", -7.9);
    assert_int_equal(ops->get_repower(&dev, &repower), 0);
    assert_close(repower, -7.0);

    int mode = -1;
    fake_preset(&fake, "WorkMode", 5.0);
    assert_int_equal(ops->get_mode(&dev, &mode), 0);
    assert_int_equal(mode, 5);

    int connect = -1;
    fake_preset(&fake, "setConnect", 1.0);
    assert_int_equal(ops->get_connect(&dev, &connect), 0);
    assert_int_equal(connect, 1);

    // v3 把 get_mode/get_connect 误派到 get_onoff；扩展接口必须读同名 tag。
    fake.read_count = 0;
    assert_int_equal(ops->get_int_value(&dev, IEMS_VENDOR_FN_GET_MODE, &mode), 0);
    assert_string_equal(fake.reads[0].tag, "WorkMode");
    assert_int_equal(ops->get_int_value(&dev, IEMS_VENDOR_FN_GET_CONNECT, &connect), 0);
    assert_string_equal(fake.reads[1].tag, "setConnect");
}

static void test_inovance_vendor(void **state) {
    (void)state;
    fake_io fake = {0};
    iems_vendor_device dev;
    const iems_vendor_semantics *ops = bind_device(&dev, &fake, "inovance");
    assert_int_equal(dev.compatible_index, 0);
    assert_int_equal(ops->init(&dev, NULL), 0);

    // RunStatu 为 0（stopped）时先写启动流程，再下发功率设定值。
    fake_preset(&fake, "RunStatu", 0.0);
    assert_int_equal(ops->set_power(&dev, 12.5), 0);
    assert_int_equal(fake.write_count, 3);
    assert_string_equal(fake.writes[0].tag, "StartCtr");
    assert_close(fake.writes[0].value, 1.0);
    assert_string_equal(fake.writes[1].tag, "StandbyCtr");
    assert_close(fake.writes[1].value, 1.0);
    assert_string_equal(fake.writes[2].tag, "ActPowSet");
    assert_close(fake.writes[2].value, 12.5);
    print_writes("inovance (start + power)", &fake);

    fake.write_count = 0;
    fake_preset(&fake, "RunStatu", 3.0);
    assert_int_equal(ops->set_power(&dev, 9.0), 0);
    assert_int_equal(ops->set_repower(&dev, -4.25), 0);
    assert_int_equal(fake.write_count, 2);
    assert_string_equal(fake.writes[0].tag, "ActPowSet");
    assert_close(fake.writes[0].value, 9.0);
    assert_string_equal(fake.writes[1].tag, "ReactPowSet");
    assert_close(fake.writes[1].value, -4.25);

    fake.write_count = 0;
    fake_preset(&fake, "RunStatu", 0.0);
    assert_int_equal(ops->set_repower(&dev, -4.25), 0);
    assert_int_equal(fake.write_count, 3);
    assert_string_equal(fake.writes[0].tag, "StartCtr");
    assert_string_equal(fake.writes[1].tag, "StandbyCtr");
    assert_string_equal(fake.writes[2].tag, "ReactPowSet");
    print_writes("inovance (start + repower)", &fake);

    // 启动流程读取失败时跳过，不阻断设定值下发。
    fake.preset_count = 0;
    fake.write_count = 0;
    assert_int_equal(ops->set_power(&dev, 5.0), 0);
    assert_int_equal(fake.write_count, 1);
    assert_string_equal(fake.writes[0].tag, "ActPowSet");

    fake.write_count = 0;
    assert_int_equal(ops->set_mode(&dev, 3), 0);
    assert_int_equal(ops->set_onoff(&dev, IEMS_VENDOR_ON), 0);
    assert_int_equal(ops->set_onoff(&dev, IEMS_VENDOR_OFF), 0);
    // v3 set_connect 空实现返回 0；按框架约定 -1 且不写。
    assert_int_equal(ops->set_connect(&dev, IEMS_VENDOR_CONNECT_GRID), -1);
    assert_int_equal(fake.write_count, 3);
    assert_string_equal(fake.writes[0].tag, "WorkMode");
    assert_close(fake.writes[0].value, 3.0);
    assert_string_equal(fake.writes[1].tag, "StartCtr");
    assert_close(fake.writes[1].value, 1.0);
    assert_string_equal(fake.writes[2].tag, "StartCtr");
    assert_close(fake.writes[2].value, 0.0);
    print_writes("inovance (mode/onoff)", &fake);

    double power = 0.0;
    fake_preset(&fake, "ActPowSet", 7.9);
    assert_int_equal(ops->get_power(&dev, &power), 0);
    assert_close(power, 7.0);
    double repower = 0.0;
    fake_preset(&fake, "ReactPowSet", -7.9);
    assert_int_equal(ops->get_repower(&dev, &repower), 0);
    assert_close(repower, -7.0);

    int mode = -1;
    fake_preset(&fake, "WorkMode", 5.0);
    assert_int_equal(ops->get_mode(&dev, &mode), 0);
    assert_int_equal(mode, 5);

    // v3 直接返回 RunStatu 原始枚举；这里映射到 OFF/STANDBY/ON。
    int onoff = -1;
    static const struct {
        int run_status;
        int expected;
    } onoff_cases[] = {
        {0, IEMS_VENDOR_OFF}, {1, IEMS_VENDOR_STANDBY}, {2, IEMS_VENDOR_ON},
        {3, IEMS_VENDOR_ON},  {4, IEMS_VENDOR_ON},
    };
    for (size_t i = 0; i < sizeof(onoff_cases) / sizeof(onoff_cases[0]); i++) {
        fake_preset(&fake, "RunStatu", (double)onoff_cases[i].run_status);
        assert_int_equal(ops->get_onoff(&dev, &onoff), 0);
        assert_int_equal(onoff, onoff_cases[i].expected);
    }
    fake_preset(&fake, "RunStatu", 9.0);
    assert_int_equal(ops->get_onoff(&dev, &onoff), -1);

    int connect = -1;
    fake_preset(&fake, "setConnect", 1.0);
    assert_int_equal(ops->get_connect(&dev, &connect), 0);
    assert_int_equal(connect, 1);

    // v3 把 get_mode/get_connect 误派到 get_onoff；扩展接口必须读同名 tag。
    fake.read_count = 0;
    assert_int_equal(ops->get_int_value(&dev, IEMS_VENDOR_FN_GET_MODE, &mode), 0);
    assert_string_equal(fake.reads[0].tag, "WorkMode");
    assert_int_equal(ops->get_int_value(&dev, IEMS_VENDOR_FN_GET_CONNECT, &connect), 0);
    assert_string_equal(fake.reads[1].tag, "setConnect");
}

static void test_nengfeng_vendor(void **state) {
    (void)state;
    fake_io fake = {0};
    iems_vendor_device dev;
    const iems_vendor_semantics *ops = bind_device(&dev, &fake, "nengfeng");
    assert_int_equal(dev.compatible_index, 0);
    assert_true(ops != iems_vendor_registry_lookup("gsstes", NULL));
    assert_int_equal(ops->init(&dev, NULL), 0);

    assert_int_equal(ops->set_power(&dev, 12.5), 0);
    assert_int_equal(ops->set_repower(&dev, -4.25), 0);
    assert_int_equal(ops->set_mode(&dev, 3), 0);
    assert_int_equal(ops->set_connect(&dev, IEMS_VENDOR_CONNECT_GRID), 0);
    assert_int_equal(ops->set_connect(&dev, IEMS_VENDOR_DISCONNECT_GRID), 0);
    assert_int_equal(ops->set_connect(&dev, 9), -1);
    assert_int_equal(fake.write_count, 5);
    assert_string_equal(fake.writes[0].tag, "ActPowSet");
    assert_close(fake.writes[0].value, 12.5);
    assert_string_equal(fake.writes[1].tag, "ReactPowSet");
    assert_close(fake.writes[1].value, -4.25);
    assert_string_equal(fake.writes[2].tag, "WorkMode");
    assert_close(fake.writes[2].value, 3.0);
    assert_string_equal(fake.writes[3].tag, "setConnect");
    assert_close(fake.writes[3].value, 0.0);
    assert_string_equal(fake.writes[4].tag, "setConnect");
    assert_close(fake.writes[4].value, 1.0);
    print_writes("nengfeng", &fake);

    // 开关机要求 Online 在线；写 0x00ff / 0xff00。
    fake.write_count = 0;
    assert_int_equal(ops->set_onoff(&dev, IEMS_VENDOR_ON), -1);
    assert_int_equal(fake.write_count, 0);
    fake_preset(&fake, "Online", 1.0);
    assert_int_equal(ops->set_onoff(&dev, IEMS_VENDOR_ON), 0);
    assert_int_equal(ops->set_onoff(&dev, IEMS_VENDOR_OFF), 0);
    assert_int_equal(ops->set_onoff(&dev, IEMS_VENDOR_STANDBY), -1);
    assert_int_equal(fake.write_count, 2);
    assert_string_equal(fake.writes[0].tag, "StartStopCtr");
    assert_close(fake.writes[0].value, 65280.0);
    assert_string_equal(fake.writes[1].tag, "StartStopCtr");
    assert_close(fake.writes[1].value, 255.0);
    print_writes("nengfeng (onoff)", &fake);

    // v3 用 int 访问器读回功率，显式截断。
    double power = 0.0;
    fake_preset(&fake, "ActPowSet", 7.9);
    assert_int_equal(ops->get_power(&dev, &power), 0);
    assert_close(power, 7.0);
    double repower = 0.0;
    fake_preset(&fake, "ReactPowSet", -7.9);
    assert_int_equal(ops->get_repower(&dev, &repower), 0);
    assert_close(repower, -7.0);

    int mode = -1;
    fake_preset(&fake, "WorkMode", 5.0);
    assert_int_equal(ops->get_mode(&dev, &mode), 0);
    assert_int_equal(mode, 5);

    int onoff = -1;
    fake_preset(&fake, "StartStopCtr", 65280.0);
    assert_int_equal(ops->get_onoff(&dev, &onoff), 0);
    assert_int_equal(onoff, IEMS_VENDOR_ON);
    fake_preset(&fake, "StartStopCtr", 255.0);
    assert_int_equal(ops->get_onoff(&dev, &onoff), 0);
    assert_int_equal(onoff, IEMS_VENDOR_OFF);
    fake_preset(&fake, "StartStopCtr", 1.0);
    assert_int_equal(ops->get_onoff(&dev, &onoff), -1);

    int connect = -1;
    fake_preset(&fake, "setConnect", 1.0);
    assert_int_equal(ops->get_connect(&dev, &connect), 0);
    assert_int_equal(connect, 1);

    // v3 把 get_mode/get_connect 误派到 get_onoff；扩展接口必须读同名 tag。
    fake.read_count = 0;
    assert_int_equal(ops->get_int_value(&dev, IEMS_VENDOR_FN_GET_MODE, &mode), 0);
    assert_string_equal(fake.reads[0].tag, "WorkMode");
    assert_int_equal(ops->get_int_value(&dev, IEMS_VENDOR_FN_GET_CONNECT, &connect), 0);
    assert_string_equal(fake.reads[1].tag, "setConnect");
}

static void test_null_port_boundaries(void **state) {
    (void)state;
    int index = -1;
    const iems_vendor_semantics *ops = iems_vendor_registry_lookup("pcs_standard", &index);
    assert_non_null(ops);

    iems_vendor_device no_io = {
        .io = NULL, .ctx = NULL, .dev_id = "dev-null-io", .compatible_index = 0};
    double power = 0.0;
    assert_int_equal(ops->set_power(&no_io, 5.0), -1);
    assert_int_equal(ops->get_power(&no_io, &power), -1);

    static const iems_vendor_io no_functions = {0};
    iems_vendor_device no_functions_dev = {
        .io = &no_functions, .ctx = NULL, .dev_id = "dev-no-fn", .compatible_index = 0};
    assert_int_equal(ops->set_power(&no_functions_dev, 5.0), -1);
    assert_int_equal(ops->get_power(&no_functions_dev, &power), -1);

    assert_int_equal(ops->set_power(NULL, 5.0), -1);
    assert_int_equal(ops->get_power(NULL, &power), -1);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_registry_aliases_and_index),
        cmocka_unit_test(test_standard_pcs),
        cmocka_unit_test(test_coepo_vendor),
        cmocka_unit_test(test_alias_index_phase_mode),
        cmocka_unit_test(test_inpower_vendor),
        cmocka_unit_test(test_gsstes_vendor),
        cmocka_unit_test(test_kelong_vendor),
        cmocka_unit_test(test_xinhuasan_vendor),
        cmocka_unit_test(test_sinexcel_vendor),
        cmocka_unit_test(test_unsigned_pow_vendor),
        cmocka_unit_test(test_xiankong_vendor),
        cmocka_unit_test(test_xinai_vendor),
        cmocka_unit_test(test_yingbo_vendor),
        cmocka_unit_test(test_zhongteng_vendor),
        cmocka_unit_test(test_three_phase_split_no_double_write),
        cmocka_unit_test(test_onoff_conventions),
        cmocka_unit_test(test_int_double_conversion),
        cmocka_unit_test(test_readonly_sum_and_unsupported),
        cmocka_unit_test(test_absolute_pv_vendor),
        cmocka_unit_test(test_absolute_mppt_vendor),
        cmocka_unit_test(test_percentage_abs_vendor),
        cmocka_unit_test(test_percentage_relative_vendor),
        cmocka_unit_test(test_mppt_ytsn_vendor),
        cmocka_unit_test(test_bms_vendor),
        cmocka_unit_test(test_grid_vendor),
        cmocka_unit_test(test_livotek_hybrid_vendor),
        cmocka_unit_test(test_enjoy_vendor),
        cmocka_unit_test(test_feilton_vendor),
        cmocka_unit_test(test_huazhi_vendor),
        cmocka_unit_test(test_inovance_vendor),
        cmocka_unit_test(test_nengfeng_vendor),
        cmocka_unit_test(test_null_port_boundaries),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
