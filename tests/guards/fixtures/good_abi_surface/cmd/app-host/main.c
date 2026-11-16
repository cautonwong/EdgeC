static NativeSymbol kSymbols[] = {
    {"point_resolve", abi_point_resolve, "(***)i", NULL},
    {"point_read", abi_point_read, "(i***)i", NULL},
    {"point_publish", abi_point_publish, "(iFiI)i", NULL},
    {"app_log", abi_log, "(i*)", NULL},
    {"app_now", abi_now, "()I", NULL},
};
