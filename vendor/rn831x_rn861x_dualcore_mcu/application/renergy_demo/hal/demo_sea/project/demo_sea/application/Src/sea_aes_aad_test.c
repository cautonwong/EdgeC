#include "sea_test.h"

/**
 * Test Scenario:
 * 1. Initialize AAD only, gcm encrypt/decrypt
 * 2. Set AAD only, gcm encrypt/decrypt
 * 3. Initialize and set AAD, gcm encrypt/decrypt
 * 4. Initialize AAD only, no message, gmac calc
 * 5. Set AAD only, no message, gmac calc
 * 6. Initialize and set AAD, no message, gmac calc
 */
#define AES_GCM_TEST_ENABLE
#define AES_GMAC_TEST_ENABLE

typedef enum
{
    AES_INIT_AAD_ONLY       = 0U, /* Only add AAD message in initialization. */
    AES_UPDATE_AAD_ONLY     = 1U, /* Only add AAD message in encryption or decryption. */
    AES_INIT_AND_UPDATE_AAD = 2U, /* Combining with the above. */
} AES_GMAC_TESTMODE;

const uint8_t aes_aad_test_pt1[] = {"87D684E7C0FC3FBF1B0B3ED69076AEC3"};
const uint8_t aes_aad_test_ky1[] = {"F0BAD348BCAD3CE36A1F29E43FAC5FC328AEB48CED0B2B60"};
const uint8_t aes_aad_test_ad1[] = {"8F1B9187CD629473C584D8EFF104E99AEE0EC01A12B71C6A0CD09D85DCEE1247BC9CBC2D87455C3903108EBA135DD059596B0BAEDC8CB94C4818CC2A9B9784533EEB4CC25E4306"
                                    "F9BD9DA7CB036583D9FB391D61425032095E1A708D3F19D7AADC911F9636CC9277CD4EFF0F951767"};
const uint8_t aes_aad_test_iv1[] = {"B9A50B44724261E55E81838E"};
const uint8_t aes_aad_test_ct1[] = {"4F6A973D7F5A7DB3FBC8569AF8421BAB"};
const uint8_t aes_aad_test_tg1[] = {"97B55DB09E748B1263860E007543C18D"};

const uint8_t aes_aad_test_pt2[] = {"B7C8B39E4067140C5EB4046306EB095D"};
const uint8_t aes_aad_test_ky2[] = {"2ACCF73AA977E68E9555B730B36E402AFF7AF802D6DFBF9F"};
const uint8_t aes_aad_test_ad2[] = {"CF02D75E8EA735FAE8617D4230627952C93C2655A703C8439B6F915CB09FA54F41761F498D2D04296069DEA9AB07FF3330C785DEEB02A0EE4C658C92D09564C70A29492F95A91A"
                                    "4E1A125848F215A8270262F21466B0798AA5B4F3A87C1E3BB8B1D1BE542E019522DCEC8F658D46A3"};
const uint8_t aes_aad_test_iv2[] = {"400E08B10C4E1698C97E7831"};
const uint8_t aes_aad_test_ct2[] = {"53B7E7B7990EF3229A3680B09A4FA9B1"};
const uint8_t aes_aad_test_tg2[] = {"36592790E23CEA477BAD88D9E6F69EFD"};

const uint8_t aes_aad_test_pt3[] = {"D8061D4BCCEE1DE52A255D7C3550965D"};
const uint8_t aes_aad_test_ky3[] = {"B28C50B033CADB95C7F2BFDA5E6B51FD6C069506550F29B5"};
const uint8_t aes_aad_test_ad3[] = {"3C964D8048AAC566256F196AF6ECEC6C75D72D0F15BCAE29FA9C218C50D5AA77EF3B9B3C4B5B2E5164D492A86279B3D474AC56CCD4FC4DD7D398DC77FB2A3AB8187CE3A9C06018"
                                    "CCA4010B629382C2D6131DB78B8DC3F3D3E45C9D6427D3537037262A254FC6F12B38E48FC8CF3233"};
const uint8_t aes_aad_test_iv3[] = {"70FD034A4E9A72DE43A13370"};
const uint8_t aes_aad_test_ct3[] = {"4420263AC4088824BC7058B3C12E6FCE"};
const uint8_t aes_aad_test_tg3[] = {"1FB35507291DE55229A7751CBC2E7E37"};

const uint8_t aes_aad_test_pt4[] = {"1C3A2D1E246131CF378DD353B9496C23"};
const uint8_t aes_aad_test_ky4[] = {"2D7BE3EDEA0352C3DDD03E163AAA5E7962B2941A4FCD7294"};
const uint8_t aes_aad_test_ad4[] = {"7C6F741D45403DC3DF55904F976639A5C10BD1196F0F18885D2393C06B65D6D55D339A710D281EB5AB125C68867ECC9E4CFE4525A306DE17E0824563F6E9D63CBD16CD630585A3"
                                    "063074204D26F80AA2868AA941A194EC2B979FF7B12E4F29CC60836091215328F540E7BA8F3175B6"};
const uint8_t aes_aad_test_iv4[] = {"D73374B320B537C01EA7EFB7"};
const uint8_t aes_aad_test_ct4[] = {"48F44629A27EF037B3AD73B19BB85B6D"};
const uint8_t aes_aad_test_tg4[] = {"866847C10A3B4900641EE775C240961B"};

const uint8_t aes_aad_test_pt5[] = {"784ECB4D41E44B88521055BDF73747A9"};
const uint8_t aes_aad_test_ky5[] = {"CA401E3B90BF669EEFA91E61D67EA807655A32BA0A77A8BE"};
const uint8_t aes_aad_test_ad5[] = {"07F4E69D2C8E53FC55E75A97583E02E13DFAF95629C0E1CE438BCE16E0DCCC8A204FB93B71D31AC382C5748F443C67B5284C2BA94E0DB2C13058704652D5C2C9769ED4C0F32B2E"
                                    "69A34763DC811226B3D120ECA2A64329F410AD67A03DBE0BE6A2F5A16AF039D615EFD1EC7702C26E"};
const uint8_t aes_aad_test_iv5[] = {"97F8AD045EBC64BF1708C5FA"};
const uint8_t aes_aad_test_ct5[] = {"D8F4AFF291001AD98969B90648486DB4"};
const uint8_t aes_aad_test_tg5[] = {"812F53C8291582AF6668012F3BA62419"};

const uint8_t aes_aad_test_pt6[] = {"5531BD8F790A91747C4BCFF68BC7A6A2"};
const uint8_t aes_aad_test_ky6[] = {"6EE1B9B604E3CBE4AC855B9177B8A96B4045E645F494F73D"};
const uint8_t aes_aad_test_ad6[] = {"64B81167397F4718D18123E67FEABE2DAA26D411881DFFDD44C2770593258E06EF86CF80EB708AB328080CD33FCF4D6CD66B837C528A043438FCD7C3337021DE5562B5CD569599"
                                    "2EA3269B8C5237DA1B86AA75B503E469A4532615849F2DDAC4955E0B19EAD3174C208761BC2F575A"};
const uint8_t aes_aad_test_iv6[] = {"4E8176F10D34B63C617CA0A1"};
const uint8_t aes_aad_test_ct6[] = {"85A8170C38780CACF0549776B2EA35F6"};
const uint8_t aes_aad_test_tg6[] = {"B6DCE3F6255C8C3FFB7B6F3AEA274CC6"};

const uint8_t aes_aad_test_pt7[] = {"D42B92DA747A0B9E3D07720C5443C335"};
const uint8_t aes_aad_test_ky7[] = {"24B2A0ADB8AF5BE8692B3A46817C06665B8899AC0E5960E2"};
const uint8_t aes_aad_test_ad7[] = {"CC23663FC18686BE1A48D1C8257D0E1D611C7BE1DC94350CE1A2ED45D6BC0CD234E304008F5FF5CF9AC3AC8560772DB19E9564609702EBD024F59FB6562182C6CE313F522FADB2"
                                    "C67D13E4C7645902805843675883DA04743634AE422672E27E8B11DF63F786F505EDE73B9B772104"};
const uint8_t aes_aad_test_iv7[] = {"D238C528399F0DD8AD9537D8"};
const uint8_t aes_aad_test_ct7[] = {"9D7D8C62F521627BED0F58B97E532E20"};
const uint8_t aes_aad_test_tg7[] = {"F25C5144EF022AA5971CA502F077EB46"};

const uint8_t aes_aad_test_pt8[] = {"FE0E7B5685FA95CE82CFE95B0A96AFCA"};
const uint8_t aes_aad_test_ky8[] = {"66D572A29D9558AE37D4F6280D941AFD5173C61D3DD25024"};
const uint8_t aes_aad_test_ad8[] = {"7D6FBF9D2D15784D4A844962D8019CD38D969326801DAA2E9025DB77CC5F2C1EA80859E84BFF0D545960CDBF9D2B600FF793CECBFD7EF6D7570B9D3B69D2AD1743E78D39119366"
                                    "5C6291A476F4013A2E61454979776F0636416B7BB15C70E5FED6C2C1D759C155410C453B04B43743"};
const uint8_t aes_aad_test_iv8[] = {"D3BCFAAC4FD084DA04BFE7FB"};
const uint8_t aes_aad_test_ct8[] = {"9D87BDD5694FF8FFEEADF17F24FBBE24"};
const uint8_t aes_aad_test_tg8[] = {"0729EB7449FA7B78D0F9627FAF7129A7"};

const uint8_t aes_aad_test_pt9[] = {"7272B8D9EC9065BA5A31C0991DA39CA8"};
const uint8_t aes_aad_test_ky9[] = {"545AD91F7152813353B9B32BA3B03DCD12BEAF12A4CEE1B1"};
const uint8_t aes_aad_test_ad9[] = {"C885B600858A285A176BA0BE64E7F86858D6356BFADB18CE4FBB1BC649DF2E02308B36874FC46D0D678A6386321AE05C062036BC238633FE0D06C4F3978FE57AF1D2D94444FE89"
                                    "8E9358F12EDA25D413AEB3552E5A20D509D07A86D852016658B93DAB072E8E0F5C4F90DDF98F0479"};
const uint8_t aes_aad_test_iv9[] = {"93DE17F4FDE79EB89DDA08D3"};
const uint8_t aes_aad_test_ct9[] = {"52EFA92A49A5B4BD61433DB8D3360A7A"};
const uint8_t aes_aad_test_tg9[] = {"03EB4BAA21D0B9149ACC9FE71035E5B5"};

const uint8_t aes_aad_test_pt10[] = {"42A1880A426F5F063246ABF0C603140B"};
const uint8_t aes_aad_test_ky10[] = {"F56F32E08C921FAA386A1326CA28C2D0AFF33AF9E7DC5953"};
const uint8_t aes_aad_test_ad10[] = {"A2A84774908967B5C08A490294620515F4F6C20DFA23DB9B2E44B50A66FE585BC14374A9E3D5A8347FB2FCC72C93FE0F3C279775FF5DEC16512B4A38BEE668BD5275969372413"
                                     "C8C6CC16AE63A2DC7797D73F1CFBBABD157D7E7EA206214F6EA9836F5F30C7C832DF65320D0B4C3FE"};
const uint8_t aes_aad_test_iv10[] = {"3C4CAD4DB4D101F44869A20F"};
const uint8_t aes_aad_test_ct10[] = {"8F9173087BEAFF993E2A185587CB4BF5"};
const uint8_t aes_aad_test_tg10[] = {"6BAC1B16FAE57E157BC2C411A45CB610"};

const uint8_t aes_aad_test_pt11[] = {"AD93FE3CA016FA497DCB345BCDF6E15E"};
const uint8_t aes_aad_test_ky11[] = {"DB2FAE7FD6C5042756ECDA242AB648215F36DF256A232EC3"};
const uint8_t aes_aad_test_ad11[] = {"F869C36E2CEAE93055A2A13EA780E67A059916485349561B90F2C0A69FC80A322399E9E2124590F34C54956C7E15F0DE3687E3912EA1D1E24BD5B3BDDE3584EB0F1F54975E1C0"
                                     "2E82B45EA242BA2F043B09EDC24B31951783148FE142DBC393778C00E6E59A07D12AC96AA7F19032A"};
const uint8_t aes_aad_test_iv11[] = {"4AEAE17A176CFDAC1D8DAD38"};
const uint8_t aes_aad_test_ct11[] = {"35ED8DF23E8F9CB5D32C73E54D350402"};
const uint8_t aes_aad_test_tg11[] = {"29FA94FAAB8A6EE0850812CC783CE439"};

const uint8_t aes_aad_test_pt12[] = {"5DD46FCAB6E5D574D69B1B42BBCD7D97"};
const uint8_t aes_aad_test_ky12[] = {"95F8B1A7851B8ACE0D1F6F44C7025F48574886355D3010C7"};
const uint8_t aes_aad_test_ad12[] = {"E4A40F70A7D81BD3C0F6807268F1929D125E68E8AD6DB48AA22C6AA4422EF45CC9B91D04D884FE49A1BB685892FA008B81A11EF1A1B57F02A9E7F5A621BD8E284E126C687D143"
                                     "E31500A9E2C6F5D7198D28C61D157436F36229B1816B64E5440E8D8B60492D318E372753511ECA19D"};
const uint8_t aes_aad_test_iv12[] = {"C2FEACF575FD76000416FF3B"};
const uint8_t aes_aad_test_ct12[] = {"8ADED0AFE90AC262D565709824F6C446"};
const uint8_t aes_aad_test_tg12[] = {"6744B00EAF2D4EE89DD4800648F67B48"};

const uint8_t aes_aad_test_pt13[] = {"1D1AF447AACF93D1032AF992D5646FE3"};
const uint8_t aes_aad_test_ky13[] = {"3AE877DD81006E9001B1366D3DF8CDBBE98C104479987DF6"};
const uint8_t aes_aad_test_ad13[] = {"5147222ECEE0CA46936E9D643899E54DCE770AD545B7A72E1711BB1974B1CCE8FDF4237222DCFAC4A24FA180287312A03D0F942723618575B9440BC9A56D9634EFC6D84B39235"
                                     "D629E2E36CA2D617222DA60860AE6BBAED63F5D957F3F99F3D91ACEE19B96ED00C41A7AE3D237C5D3"};
const uint8_t aes_aad_test_iv13[] = {"4FC44B629846B003D1616E39"};
const uint8_t aes_aad_test_ct13[] = {"56AE36FD60F7ADC8D3E827D0B685F2C7"};
const uint8_t aes_aad_test_tg13[] = {"0B6992D021E83FBF2426DFD8FCC685DF"};

const uint8_t aes_aad_test_pt14[] = {"FE68CDCBA2C3065450EBE92568BC2B3A"};
const uint8_t aes_aad_test_ky14[] = {"9365F2ABB4E6DA7429BD2D7E2271195863CA00E0CD4A8C3430AB2F534E7CCDA8"};
const uint8_t aes_aad_test_ad14[] = {"5307DF9CAE24F73A5ACED13BF829DF3932B3C565E83E4F0FDD8E30693AFF02245018A2274B3D75C7AD225AD2EA80C6857DB8B4095753A4F62F2AD924EBA45DB5A0BBBCBFAF5DF"
                                     "2E6BC1DB98C357E3AB6B21B0D98B5A2DF0CD752853D2332E466AE8F50253679A18B5069AB375F7B48B468FD56CF0A0C45F1E0F23851E9424F68"};
const uint8_t aes_aad_test_iv14[] = {"5418077D33A2B308EB18A4FA94C0FF0FA46E7F739A387FB60D83C7EA828CA3F46340F75F8AEC22E771F00789C25375A8DDBADE8C37B3A84B77926A568A24F9F2A1664D0968DDC"
                                     "982FBB7274284F64878DA520F7B7F455BB1495F3C546F1D9F98338B800BCC4FFBED365ED2"};
const uint8_t aes_aad_test_ct14[] = {"4BC26A8CC11E131E5F40D46F87AE4C42"};
const uint8_t aes_aad_test_tg14[] = {"399B89E3144FF9EE"};

const uint8_t aes_aad_test_pt15[] = {"EBB0E55E9FF2DBB9180B4DAC5A70DB40"};
const uint8_t aes_aad_test_ky15[] = {"2471D0DDCA33099BAEA25C0DA692F5192B56B694BD33F275EBEB79670E9D3D43"};
const uint8_t aes_aad_test_ad15[] = {"6F99E9EF1B87635470EA7B1E5741C0AA5E39491249946FE855333F41745BECDDA13F9DF8ECAD01CE4F14FC64120EE8AA755F178D0EDDA40D91D59C1C56B3696C4F8D2D40EB350"
                                     "A76C757F0EDA200BDF15096F4DEC83B6BCF80892DF623C9263F029E65D00039406A3DDFC979AC2496BFCBBFE67DF36C0BB55E2D792CA0485590"};
const uint8_t aes_aad_test_iv15[] = {"D19C1239C089B13613E84AD3F0CAFCFAB6CE3E7859F8DB14D7255AB170545288B2BA44A81E4D8D0A384075CE80E2DAB9DFEA80F2D0084A154530864D94E97789B7CBB5CB83DCE"
                                     "436C00F81A40E20455D73EE9BE7DD09536885C78503834E19B9E484EFC63F4DA87AD15DFC"};
const uint8_t aes_aad_test_ct15[] = {"A7404DE432C03E6F66FFA1ECC00757D4"};
const uint8_t aes_aad_test_tg15[] = {"19663C202B80E5D3"};

const uint8_t aes_aad_test_pt16[] = {""};
const uint8_t aes_aad_test_ky16[] = {"A0D83382D913895F32AFBBBFB94CA91D"};
const uint8_t aes_aad_test_ad16[] = {"F4594739A94AEA9E3BBA8FEDBE04DBBE5196FABF882AA0A6A16AC8E6EFC72951E229AB7FEB4830C99EC1F7F52BD2FA47CE5FE38F"};
const uint8_t aes_aad_test_iv16[] = {"5D9418"};
const uint8_t aes_aad_test_ct16[] = {""};
const uint8_t aes_aad_test_tg16[] = {"52FE67F75AE6BD66C73B720AC1680B"};

const uint8_t aes_aad_test_pt17[] = {""};
const uint8_t aes_aad_test_ky17[] = {"1F19503EDC1C78EECBDA3906ECBC9C9F"};
const uint8_t aes_aad_test_ad17[] = {"F74A0CBE95B6B003059727283D57E2E556BB9BF12C96D4B7C8F99E509FFD81A6967511"};
const uint8_t aes_aad_test_iv17[] = {"A08559C3386F154AA7B5F43B96E8F5E24A51CB6F09EB911758EF"};
const uint8_t aes_aad_test_ct17[] = {""};
const uint8_t aes_aad_test_tg17[] = {"8D636459677E544F2E78EF8FF5"};

const uint8_t aes_aad_test_pt18[] = {""};
const uint8_t aes_aad_test_ky18[] = {"8344816E93546368F6BF3ACF4FE16C77"};
const uint8_t aes_aad_test_ad18[] = {"B4B82548569E4636BA14BBFC13BB7184CD77FDED1B9699BE91F38BB39B8359FFF3BAB853AC95184A313C1A73BF610E4F508AF559AA9E7F6C0CD1DE6EE97873D5887A5E210BC20"
                                     "EC2D0A78C9B02A5906F62752B0F58E17A52DCC315BB58E9F2BDD81D194FD170C47C994D3B24F05B2F228D3658DB354CF2A2CA2F92D03EA8100C"};
const uint8_t aes_aad_test_iv18[] = {"CC7DF732C71DA6BB51615B4B"};
const uint8_t aes_aad_test_ct18[] = {""};
const uint8_t aes_aad_test_tg18[] = {"79EBE4D46CBD21DC4EEDE72922D8"};

const uint8_t aes_aad_test_pt19[] = {""};
const uint8_t aes_aad_test_ky19[] = {"17979378F909FB32F6D6D8CF1DA347F6EE1D569E0A285515"};
const uint8_t aes_aad_test_ad19[] = {"8C62CB509144E7B2B457DA6D8C5036A6B43F3CF1FC8BD896F60AD0E939B84612A0CA24CE2C9139D10AB5B95D6BBFEAC4FBF55B1566B44AB131F7E2CA16F2686DB4452D7614319"
                                     "B0F55A3A5463C0A8DD9D5E6DCA225FB58CD413900C8A18E74B0FCF567F998BFABD7978111BE828CEA31C084598BA365631C3FC536C009339067"};
const uint8_t aes_aad_test_iv19[] = {"6127D3273C4E5D2F727F0789"};
const uint8_t aes_aad_test_ct19[] = {""};
const uint8_t aes_aad_test_tg19[] = {"D7ACFA6889C61B4C"};

const uint8_t aes_aad_test_pt20[] = {""};
const uint8_t aes_aad_test_ky20[] = {"81E975A14A38B04EB4E1D2C9FF647886F3D050CB7D1A6FA49BB5147452D71376"};
const uint8_t aes_aad_test_ad20[] = {"0989FBF4832AD28D8332D5EAAF2D938E78F9A240178BFFBA8D4C74DA7D3C98852EFA6BDCC3042331A3C1BD4F138BF11CA422D6BA"};
const uint8_t aes_aad_test_iv20[] = {"3F92B17465BEF9A75C764E40"};
const uint8_t aes_aad_test_ct20[] = {""};
const uint8_t aes_aad_test_tg20[] = {"F3A82B6F0F154AB88EDDC4AE649B09"};

const SEA_AES_AAD_TESTLIST aes_aad_test1 = {(uint32_t *)aes_aad_test_pt1,
                                            (uint32_t)sizeof(aes_aad_test_pt1) >> 1,
                                            (uint32_t *)aes_aad_test_ky1,
                                            (uint32_t)sizeof(aes_aad_test_ky1) >> 1,
                                            (uint32_t *)aes_aad_test_iv1,
                                            (uint32_t)sizeof(aes_aad_test_iv1) >> 1,
                                            (uint32_t *)aes_aad_test_ad1,
                                            (uint32_t)sizeof(aes_aad_test_ad1) >> 1,
                                            (uint32_t *)aes_aad_test_ct1,
                                            (uint32_t *)aes_aad_test_tg1,
                                            (uint32_t)sizeof(aes_aad_test_tg1) >> 1,
                                            AES_192,
                                            AES_GCM};

const SEA_AES_AAD_TESTLIST aes_aad_test2 = {(uint32_t *)aes_aad_test_pt2,
                                            (uint32_t)sizeof(aes_aad_test_pt2) >> 1,
                                            (uint32_t *)aes_aad_test_ky2,
                                            (uint32_t)sizeof(aes_aad_test_ky2) >> 1,
                                            (uint32_t *)aes_aad_test_iv2,
                                            (uint32_t)sizeof(aes_aad_test_iv2) >> 1,
                                            (uint32_t *)aes_aad_test_ad2,
                                            (uint32_t)sizeof(aes_aad_test_ad2) >> 1,
                                            (uint32_t *)aes_aad_test_ct2,
                                            (uint32_t *)aes_aad_test_tg2,
                                            (uint32_t)sizeof(aes_aad_test_tg2) >> 1,
                                            AES_192,
                                            AES_GCM};
const SEA_AES_AAD_TESTLIST aes_aad_test3 = {(uint32_t *)aes_aad_test_pt3,
                                            (uint32_t)sizeof(aes_aad_test_pt3) >> 1,
                                            (uint32_t *)aes_aad_test_ky3,
                                            (uint32_t)sizeof(aes_aad_test_ky3) >> 1,
                                            (uint32_t *)aes_aad_test_iv3,
                                            (uint32_t)sizeof(aes_aad_test_iv3) >> 1,
                                            (uint32_t *)aes_aad_test_ad3,
                                            (uint32_t)sizeof(aes_aad_test_ad3) >> 1,
                                            (uint32_t *)aes_aad_test_ct3,
                                            (uint32_t *)aes_aad_test_tg3,
                                            (uint32_t)sizeof(aes_aad_test_tg3) >> 1,
                                            AES_192,
                                            AES_GCM};
const SEA_AES_AAD_TESTLIST aes_aad_test4 = {(uint32_t *)aes_aad_test_pt4,
                                            (uint32_t)sizeof(aes_aad_test_pt4) >> 1,
                                            (uint32_t *)aes_aad_test_ky4,
                                            (uint32_t)sizeof(aes_aad_test_ky4) >> 1,
                                            (uint32_t *)aes_aad_test_iv4,
                                            (uint32_t)sizeof(aes_aad_test_iv4) >> 1,
                                            (uint32_t *)aes_aad_test_ad4,
                                            (uint32_t)sizeof(aes_aad_test_ad4) >> 1,
                                            (uint32_t *)aes_aad_test_ct4,
                                            (uint32_t *)aes_aad_test_tg4,
                                            (uint32_t)sizeof(aes_aad_test_tg4) >> 1,
                                            AES_192,
                                            AES_GCM};
const SEA_AES_AAD_TESTLIST aes_aad_test5 = {(uint32_t *)aes_aad_test_pt5,
                                            (uint32_t)sizeof(aes_aad_test_pt5) >> 1,
                                            (uint32_t *)aes_aad_test_ky5,
                                            (uint32_t)sizeof(aes_aad_test_ky5) >> 1,
                                            (uint32_t *)aes_aad_test_iv5,
                                            (uint32_t)sizeof(aes_aad_test_iv5) >> 1,
                                            (uint32_t *)aes_aad_test_ad5,
                                            (uint32_t)sizeof(aes_aad_test_ad5) >> 1,
                                            (uint32_t *)aes_aad_test_ct5,
                                            (uint32_t *)aes_aad_test_tg5,
                                            (uint32_t)sizeof(aes_aad_test_tg5) >> 1,
                                            AES_192,
                                            AES_GCM};
const SEA_AES_AAD_TESTLIST aes_aad_test6 = {(uint32_t *)aes_aad_test_pt6,
                                            (uint32_t)sizeof(aes_aad_test_pt6) >> 1,
                                            (uint32_t *)aes_aad_test_ky6,
                                            (uint32_t)sizeof(aes_aad_test_ky6) >> 1,
                                            (uint32_t *)aes_aad_test_iv6,
                                            (uint32_t)sizeof(aes_aad_test_iv6) >> 1,
                                            (uint32_t *)aes_aad_test_ad6,
                                            (uint32_t)sizeof(aes_aad_test_ad6) >> 1,
                                            (uint32_t *)aes_aad_test_ct6,
                                            (uint32_t *)aes_aad_test_tg6,
                                            (uint32_t)sizeof(aes_aad_test_tg6) >> 1,
                                            AES_192,
                                            AES_GCM};
const SEA_AES_AAD_TESTLIST aes_aad_test7 = {(uint32_t *)aes_aad_test_pt7,
                                            (uint32_t)sizeof(aes_aad_test_pt7) >> 1,
                                            (uint32_t *)aes_aad_test_ky7,
                                            (uint32_t)sizeof(aes_aad_test_ky7) >> 1,
                                            (uint32_t *)aes_aad_test_iv7,
                                            (uint32_t)sizeof(aes_aad_test_iv7) >> 1,
                                            (uint32_t *)aes_aad_test_ad7,
                                            (uint32_t)sizeof(aes_aad_test_ad7) >> 1,
                                            (uint32_t *)aes_aad_test_ct7,
                                            (uint32_t *)aes_aad_test_tg7,
                                            (uint32_t)sizeof(aes_aad_test_tg7) >> 1,
                                            AES_192,
                                            AES_GCM};
const SEA_AES_AAD_TESTLIST aes_aad_test8 = {(uint32_t *)aes_aad_test_pt8,
                                            (uint32_t)sizeof(aes_aad_test_pt8) >> 1,
                                            (uint32_t *)aes_aad_test_ky8,
                                            (uint32_t)sizeof(aes_aad_test_ky8) >> 1,
                                            (uint32_t *)aes_aad_test_iv8,
                                            (uint32_t)sizeof(aes_aad_test_iv8) >> 1,
                                            (uint32_t *)aes_aad_test_ad8,
                                            (uint32_t)sizeof(aes_aad_test_ad8) >> 1,
                                            (uint32_t *)aes_aad_test_ct8,
                                            (uint32_t *)aes_aad_test_tg8,
                                            (uint32_t)sizeof(aes_aad_test_tg8) >> 1,
                                            AES_192,
                                            AES_GCM};
const SEA_AES_AAD_TESTLIST aes_aad_test9 = {(uint32_t *)aes_aad_test_pt9,
                                            (uint32_t)sizeof(aes_aad_test_pt9) >> 1,
                                            (uint32_t *)aes_aad_test_ky9,
                                            (uint32_t)sizeof(aes_aad_test_ky9) >> 1,
                                            (uint32_t *)aes_aad_test_iv9,
                                            (uint32_t)sizeof(aes_aad_test_iv9) >> 1,
                                            (uint32_t *)aes_aad_test_ad9,
                                            (uint32_t)sizeof(aes_aad_test_ad9) >> 1,
                                            (uint32_t *)aes_aad_test_ct9,
                                            (uint32_t *)aes_aad_test_tg9,
                                            (uint32_t)sizeof(aes_aad_test_tg9) >> 1,
                                            AES_192,
                                            AES_GCM};
const SEA_AES_AAD_TESTLIST aes_aad_test10 = {(uint32_t *)aes_aad_test_pt10,
                                             (uint32_t)sizeof(aes_aad_test_pt10) >> 1,
                                             (uint32_t *)aes_aad_test_ky10,
                                             (uint32_t)sizeof(aes_aad_test_ky10) >> 1,
                                             (uint32_t *)aes_aad_test_iv10,
                                             (uint32_t)sizeof(aes_aad_test_iv10) >> 1,
                                             (uint32_t *)aes_aad_test_ad10,
                                             (uint32_t)sizeof(aes_aad_test_ad10) >> 1,
                                             (uint32_t *)aes_aad_test_ct10,
                                             (uint32_t *)aes_aad_test_tg10,
                                             (uint32_t)sizeof(aes_aad_test_tg10) >> 1,
                                             AES_192,
                                             AES_GCM};
const SEA_AES_AAD_TESTLIST aes_aad_test11 = {(uint32_t *)aes_aad_test_pt11,
                                             (uint32_t)sizeof(aes_aad_test_pt11) >> 1,
                                             (uint32_t *)aes_aad_test_ky11,
                                             (uint32_t)sizeof(aes_aad_test_ky11) >> 1,
                                             (uint32_t *)aes_aad_test_iv11,
                                             (uint32_t)sizeof(aes_aad_test_iv11) >> 1,
                                             (uint32_t *)aes_aad_test_ad11,
                                             (uint32_t)sizeof(aes_aad_test_ad11) >> 1,
                                             (uint32_t *)aes_aad_test_ct11,
                                             (uint32_t *)aes_aad_test_tg11,
                                             (uint32_t)sizeof(aes_aad_test_tg11) >> 1,
                                             AES_192,
                                             AES_GCM};
const SEA_AES_AAD_TESTLIST aes_aad_test12 = {(uint32_t *)aes_aad_test_pt12,
                                             (uint32_t)sizeof(aes_aad_test_pt12) >> 1,
                                             (uint32_t *)aes_aad_test_ky12,
                                             (uint32_t)sizeof(aes_aad_test_ky12) >> 1,
                                             (uint32_t *)aes_aad_test_iv12,
                                             (uint32_t)sizeof(aes_aad_test_iv12) >> 1,
                                             (uint32_t *)aes_aad_test_ad12,
                                             (uint32_t)sizeof(aes_aad_test_ad12) >> 1,
                                             (uint32_t *)aes_aad_test_ct12,
                                             (uint32_t *)aes_aad_test_tg12,
                                             (uint32_t)sizeof(aes_aad_test_tg12) >> 1,
                                             AES_192,
                                             AES_GCM};
const SEA_AES_AAD_TESTLIST aes_aad_test13 = {(uint32_t *)aes_aad_test_pt13,
                                             (uint32_t)sizeof(aes_aad_test_pt13) >> 1,
                                             (uint32_t *)aes_aad_test_ky13,
                                             (uint32_t)sizeof(aes_aad_test_ky13) >> 1,
                                             (uint32_t *)aes_aad_test_iv13,
                                             (uint32_t)sizeof(aes_aad_test_iv13) >> 1,
                                             (uint32_t *)aes_aad_test_ad13,
                                             (uint32_t)sizeof(aes_aad_test_ad13) >> 1,
                                             (uint32_t *)aes_aad_test_ct13,
                                             (uint32_t *)aes_aad_test_tg13,
                                             (uint32_t)sizeof(aes_aad_test_tg13) >> 1,
                                             AES_192,
                                             AES_GCM};
const SEA_AES_AAD_TESTLIST aes_aad_test14 = {(uint32_t *)aes_aad_test_pt14,
                                             (uint32_t)sizeof(aes_aad_test_pt14) >> 1,
                                             (uint32_t *)aes_aad_test_ky14,
                                             (uint32_t)sizeof(aes_aad_test_ky14) >> 1,
                                             (uint32_t *)aes_aad_test_iv14,
                                             (uint32_t)sizeof(aes_aad_test_iv14) >> 1,
                                             (uint32_t *)aes_aad_test_ad14,
                                             (uint32_t)sizeof(aes_aad_test_ad14) >> 1,
                                             (uint32_t *)aes_aad_test_ct14,
                                             (uint32_t *)aes_aad_test_tg14,
                                             (uint32_t)sizeof(aes_aad_test_tg14) >> 1,
                                             AES_256,
                                             AES_GCM};
const SEA_AES_AAD_TESTLIST aes_aad_test15 = {(uint32_t *)aes_aad_test_pt15,
                                             (uint32_t)sizeof(aes_aad_test_pt15) >> 1,
                                             (uint32_t *)aes_aad_test_ky15,
                                             (uint32_t)sizeof(aes_aad_test_ky15) >> 1,
                                             (uint32_t *)aes_aad_test_iv15,
                                             (uint32_t)sizeof(aes_aad_test_iv15) >> 1,
                                             (uint32_t *)aes_aad_test_ad15,
                                             (uint32_t)sizeof(aes_aad_test_ad15) >> 1,
                                             (uint32_t *)aes_aad_test_ct15,
                                             (uint32_t *)aes_aad_test_tg15,
                                             (uint32_t)sizeof(aes_aad_test_tg15) >> 1,
                                             AES_256,
                                             AES_GCM};
const SEA_AES_AAD_TESTLIST aes_aad_test16 = {(uint32_t *)0,
                                             (uint32_t)0,
                                             (uint32_t *)aes_aad_test_ky16,
                                             (uint32_t)sizeof(aes_aad_test_ky16) >> 1,
                                             (uint32_t *)aes_aad_test_iv16,
                                             (uint32_t)sizeof(aes_aad_test_iv16) >> 1,
                                             (uint32_t *)aes_aad_test_ad16,
                                             (uint32_t)sizeof(aes_aad_test_ad16) >> 1,
                                             (uint32_t *)0,
                                             (uint32_t *)aes_aad_test_tg16,
                                             (uint32_t)sizeof(aes_aad_test_tg16) >> 1,
                                             AES_128,
                                             AES_GCM};
const SEA_AES_AAD_TESTLIST aes_aad_test17 = {(uint32_t *)0,
                                             (uint32_t)0,
                                             (uint32_t *)aes_aad_test_ky17,
                                             (uint32_t)sizeof(aes_aad_test_ky17) >> 1,
                                             (uint32_t *)aes_aad_test_iv17,
                                             (uint32_t)sizeof(aes_aad_test_iv17) >> 1,
                                             (uint32_t *)aes_aad_test_ad17,
                                             (uint32_t)sizeof(aes_aad_test_ad17) >> 1,
                                             (uint32_t *)0,
                                             (uint32_t *)aes_aad_test_tg17,
                                             (uint32_t)sizeof(aes_aad_test_tg17) >> 1,
                                             AES_128,
                                             AES_GCM};
const SEA_AES_AAD_TESTLIST aes_aad_test18 = {(uint32_t *)0,
                                             (uint32_t)0,
                                             (uint32_t *)aes_aad_test_ky18,
                                             (uint32_t)sizeof(aes_aad_test_ky18) >> 1,
                                             (uint32_t *)aes_aad_test_iv18,
                                             (uint32_t)sizeof(aes_aad_test_iv18) >> 1,
                                             (uint32_t *)aes_aad_test_ad18,
                                             (uint32_t)sizeof(aes_aad_test_ad18) >> 1,
                                             (uint32_t *)0,
                                             (uint32_t *)aes_aad_test_tg18,
                                             (uint32_t)sizeof(aes_aad_test_tg18) >> 1,
                                             AES_128,
                                             AES_GCM};
const SEA_AES_AAD_TESTLIST aes_aad_test19 = {(uint32_t *)0,
                                             (uint32_t)0,
                                             (uint32_t *)aes_aad_test_ky19,
                                             (uint32_t)sizeof(aes_aad_test_ky19) >> 1,
                                             (uint32_t *)aes_aad_test_iv19,
                                             (uint32_t)sizeof(aes_aad_test_iv19) >> 1,
                                             (uint32_t *)aes_aad_test_ad19,
                                             (uint32_t)sizeof(aes_aad_test_ad19) >> 1,
                                             (uint32_t *)0,
                                             (uint32_t *)aes_aad_test_tg19,
                                             (uint32_t)sizeof(aes_aad_test_tg19) >> 1,
                                             AES_192,
                                             AES_GCM};
const SEA_AES_AAD_TESTLIST aes_aad_test20 = {(uint32_t *)0,
                                             (uint32_t)0,
                                             (uint32_t *)aes_aad_test_ky20,
                                             (uint32_t)sizeof(aes_aad_test_ky20) >> 1,
                                             (uint32_t *)aes_aad_test_iv20,
                                             (uint32_t)sizeof(aes_aad_test_iv20) >> 1,
                                             (uint32_t *)aes_aad_test_ad20,
                                             (uint32_t)sizeof(aes_aad_test_ad20) >> 1,
                                             (uint32_t *)0,
                                             (uint32_t *)aes_aad_test_tg20,
                                             (uint32_t)sizeof(aes_aad_test_tg20) >> 1,
                                             AES_256,
                                             AES_GCM};

const SEA_AES_AAD_TESTLIST *aes_aad_testlist[] = {
    (SEA_AES_AAD_TESTLIST *)&aes_aad_test1,  (SEA_AES_AAD_TESTLIST *)&aes_aad_test2,  (SEA_AES_AAD_TESTLIST *)&aes_aad_test3,  (SEA_AES_AAD_TESTLIST *)&aes_aad_test4,
    (SEA_AES_AAD_TESTLIST *)&aes_aad_test5,  (SEA_AES_AAD_TESTLIST *)&aes_aad_test6,  (SEA_AES_AAD_TESTLIST *)&aes_aad_test7,  (SEA_AES_AAD_TESTLIST *)&aes_aad_test8,
    (SEA_AES_AAD_TESTLIST *)&aes_aad_test9,  (SEA_AES_AAD_TESTLIST *)&aes_aad_test10, (SEA_AES_AAD_TESTLIST *)&aes_aad_test11, (SEA_AES_AAD_TESTLIST *)&aes_aad_test12,
    (SEA_AES_AAD_TESTLIST *)&aes_aad_test13, (SEA_AES_AAD_TESTLIST *)&aes_aad_test14, (SEA_AES_AAD_TESTLIST *)&aes_aad_test15, (SEA_AES_AAD_TESTLIST *)&aes_aad_test16,
    (SEA_AES_AAD_TESTLIST *)&aes_aad_test17, (SEA_AES_AAD_TESTLIST *)&aes_aad_test18, (SEA_AES_AAD_TESTLIST *)&aes_aad_test19, (SEA_AES_AAD_TESTLIST *)&aes_aad_test20,
};

#define AES_GCM_AAD_TESTTIMES 15
#define AES_GMAC_AAD_TESTTIMES 5

static uint32_t aes_gcm_testunit(SEA_AES_AAD_TESTLIST *testlist, AES_ENCRYPT_SEL encrypt_sel, AES_GMAC_TESTMODE aad_mode) {
    uint32_t tmp_err = SEA_SUCCESS;
#ifdef AES_PROCESS_LATCH_RECOVER_SUPPORT
    aesProcessParameters tmp_latch_buf;
#endif
    uint32_t aad[64];
    uint32_t pt[32], ct[32], key[8], iv[64], tagSign[8], tagVer[8], output[32];
    uint32_t i, total_len, loops, calcSize, tmp_addr;
    AES_InitTypeDef cfg;
    aesProcessParameters pParas;
    AES_VERIFY_STA verStat;
    SEA_AES_AAD_TESTLIST *pCase;
    pCase = (SEA_AES_AAD_TESTLIST *)testlist;

    sea_memset_word(pt, 0U, 32U);
    sea_memset_word(ct, 0U, 32U);
    sea_memset_word(output, 0U, 32U);
    sea_memset_word(key, 0U, 8U);
    sea_memset_word(iv, 0U, 64U);
    sea_memset_word(tagSign, 0U, 8U);
    sea_memset_word(tagVer, 0U, 8U);
    sea_memset_word(aad, 0U, 64U);

    str2hex((const uint8_t *)pCase->key, (uint8_t *)key, pCase->keylen);
    str2hex((const uint8_t *)pCase->iv, (uint8_t *)iv, pCase->ivlen);
    str2hex((const uint8_t *)pCase->aad, (uint8_t *)aad, pCase->aadlen);

    cfg.encrypt_sel = encrypt_sel;
    cfg.bitWideMode = pCase->bitWideMode;
    cfg.cryptMode = pCase->cryptMode;
    cfg.keyLocation = (aesKey_t *)&key[0];
    cfg.ivLocation = (aesIV_t *)&iv[0];
    cfg.ivSize = pCase->ivlen;
    if (aad_mode == AES_UPDATE_AAD_ONLY) {
        cfg.aadLocation = (aesAAD_t *)0U;
        cfg.aadSize = 0U;
    } else if (aad_mode == AES_INIT_AND_UPDATE_AAD) {
        cfg.aadLocation = (aesAAD_t *)&aad[0]; /* 16 bytes AAD message are passed in at a time. */
        cfg.aadSize = 16U;
    } else {
        cfg.aadLocation = (aesAAD_t *)&aad[0];
        cfg.aadSize = pCase->aadlen;
    }
    cfg.tagSignBuf = (aesTag_t *)&tagSign[0];
    cfg.tagVerifyBuf = (aesTag_t *)&tagVer[0];
    cfg.tagSize = pCase->taglen;
    cfg.tagVerifySta = &verStat;
    cfg.specialMode = AES_SPC_NONE;
#ifndef SEA_HAL_FULL_TEST
    cfg.assignedAddress = (uint32_t)&AES_GCM_DMA_TEST[0];
#endif

    str2hex((const uint8_t *)pCase->pt, (uint8_t *)pt, pCase->ptlen);
    str2hex((const uint8_t *)pCase->ct, (uint8_t *)ct, pCase->ptlen);
    if (cfg.encrypt_sel == AES_deCrypt) {
        str2hex((const uint8_t *)pCase->tag, (uint8_t *)tagSign, cfg.tagSize);
    } else {
        str2hex((const uint8_t *)pCase->tag, (uint8_t *)tagVer, cfg.tagSize);
    }

    tmp_err = LL_AES_crypt_init(&pParas, (const AES_InitTypeDef *)&cfg);
    TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);

    if (aad_mode != AES_INIT_AAD_ONLY) {
        if (aad_mode == AES_INIT_AND_UPDATE_AAD) {
            total_len = (pCase->aadlen - 16);
        } else {
            total_len = pCase->aadlen;
        }
        loops = ((0 != (total_len % 16U)) ? ((total_len >> 4U) + 1U) : (total_len >> 4U));
        for (i = 0; i < loops; i++) {
            WDT_FEED;
#ifdef AES_PROCESS_LATCH_RECOVER_SUPPORT
            if ((i == 0) || (i == 3)) {
                printf("[log]++aes_latch recover begin\n");
                LL_AES_process_status_latch(&pParas, &tmp_latch_buf);
                aes_128_gcm_demo_init(&pParas, AES_enCrypt);
                aes_128_gcm_demo_crypt_sign(&pParas);
                LL_AES_process_status_recover(&tmp_latch_buf, &pParas);
                printf("[log]++aes_latch recover finish\n");
            } else if (i == 1) {
                printf("[log]++aes_latch recover begin\n");
                LL_AES_process_status_latch(&pParas, &tmp_latch_buf);
                aes_128_gcm_demo_init(&pParas, AES_deCrypt);
                aes_128_gcm_demo_decrypt_verify(&pParas);
                LL_AES_process_status_recover(&tmp_latch_buf, &pParas);
                printf("[log]++aes_latch recover finish\n");
            }
#endif
            calcSize = ((total_len > 16U) ? 16U : total_len);
            total_len -= calcSize;

            if (aad_mode == AES_INIT_AND_UPDATE_AAD) {
                tmp_addr = (uint32_t)((uint8_t *)aad + 16U + i * 16U);
            } else {
                tmp_addr = (uint32_t)((uint8_t *)aad + i * 16U);
            }
            tmp_err = LL_AES_update_aad(&pParas, (const aesAAD_t *)tmp_addr, calcSize);
            TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);
        }
    }

    if (AES_enCrypt == encrypt_sel) {
        tmp_err = LL_AES_crypt_sign(&pParas, (const PlainData_t *)pt, pCase->ptlen, CypherDatasIsTheLast, (CryptData_t *)output);
    } else {
        tmp_err = LL_AES_decrypt_verify(&pParas, (const CryptData_t *)ct, pCase->ptlen, CypherDatasIsTheLast, (PlainData_t *)output);
    }
    TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);
    (void)LL_AES_crypt_deinit();

    if (AES_enCrypt == encrypt_sel) {
        tmp_err = memcmp((const uint8_t *)output, (const uint8_t *)ct, pCase->ptlen);
        if (0 != tmp_err) {
            printf("[log]++output\tct\n");
            for (i = 0; i < (pCase->ptlen >> 2U); i++) {
                printf("%08X %08X\n", output[i], ct[i]);
            }
        }
        TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);

        tmp_err = memcmp((const uint8_t *)tagSign, (const uint8_t *)tagVer, pCase->taglen);
        if (0 != tmp_err) {
            printf("[log]++tagSign\ttagVer\n");
            for (i = 0; i < (pCase->taglen >> 2U); i++) {
                printf("%08X %08X\n", tagSign[i], tagVer[i]);
            }
        }
        TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);
    } else {
        tmp_err = memcmp((const uint8_t *)output, (const uint8_t *)pt, pCase->ptlen);
        if (0 != tmp_err) {
            printf("[log]++output\tct\n");
            for (i = 0; i < (pCase->ptlen >> 2U); i++) {
                printf("%08X %08X\n", output[i], pt[i]);
            }
        }
        TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);

        tmp_err = memcmp((const uint8_t *)tagVer, (const uint8_t *)tagSign, pCase->taglen);
        if (0 != tmp_err) {
            printf("[log]++tagSign\ttagVer\n");
            for (i = 0; i < (pCase->taglen >> 2U); i++) {
                printf("%08X %08X\n", tagSign[i], tagVer[i]);
            }
        }
        TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);
        TEST_ASSERT_EQUAL_UINT(AES_VERIFY_SUCCESS, verStat);
    }

    return tmp_err;
}

static uint32_t aes_gmac_testunit(SEA_AES_AAD_TESTLIST *testlist, AES_ENCRYPT_SEL encrypt_sel, AES_GMAC_TESTMODE aad_mode) {
    uint32_t tmp_err = SEA_SUCCESS;
#ifdef AES_PROCESS_LATCH_RECOVER_SUPPORT
    aesProcessParameters tmp_latch_buf;
#endif
    uint32_t aad[64] = {0};
    uint32_t key[12] = {0};
    uint32_t iv[64] = {0};
    uint32_t tagSign[8] = {0};
    uint32_t tagVer[8] = {0};
    uint32_t i, total_len, loops, calcSize, tmp_addr;
    AES_InitTypeDef cfg;
    aesProcessParameters pParas;
    AES_VERIFY_STA verStat;
    SEA_AES_AAD_TESTLIST *pCase;
    pCase = (SEA_AES_AAD_TESTLIST *)testlist;

    str2hex((const uint8_t *)pCase->key, (uint8_t *)key, pCase->keylen);
    str2hex((const uint8_t *)pCase->iv, (uint8_t *)iv, pCase->ivlen);
    str2hex((const uint8_t *)pCase->aad, (uint8_t *)aad, pCase->aadlen);

    cfg.encrypt_sel = encrypt_sel;
    cfg.bitWideMode = pCase->bitWideMode;
    cfg.cryptMode = pCase->cryptMode;
    cfg.keyLocation = (aesKey_t *)&key[0];
    cfg.ivLocation = (aesIV_t *)&iv[0];
    cfg.ivSize = pCase->ivlen;
    if (aad_mode == AES_UPDATE_AAD_ONLY) {
        cfg.aadLocation = (aesAAD_t *)0U;
        cfg.aadSize = 0U;
    } else if (aad_mode == AES_INIT_AND_UPDATE_AAD) {
        cfg.aadLocation = (aesAAD_t *)&aad[0]; /* 16 bytes AAD message are passed in at a time. */
        cfg.aadSize = 16U;
    } else {
        cfg.aadLocation = (aesAAD_t *)&aad[0];
        cfg.aadSize = pCase->aadlen;
    }
    cfg.tagSignBuf = (aesTag_t *)&tagSign[0];
    cfg.tagVerifyBuf = (aesTag_t *)&tagVer[0];
    cfg.tagSize = pCase->taglen;
    cfg.tagVerifySta = &verStat;
    cfg.specialMode = AES_SPC_NONE;
#ifndef SEA_HAL_FULL_TEST
    cfg.assignedAddress = (uint32_t)&AES_GCM_DMA_TEST[0];
#endif

    if (cfg.encrypt_sel == AES_deCrypt) {
        str2hex((const uint8_t *)pCase->tag, (uint8_t *)tagSign, cfg.tagSize);
    } else {
        str2hex((const uint8_t *)pCase->tag, (uint8_t *)tagVer, cfg.tagSize);
    }

    tmp_err = LL_AES_crypt_init(&pParas, (const AES_InitTypeDef *)&cfg);
    TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);

    if (aad_mode != AES_INIT_AAD_ONLY) {
        if (aad_mode == AES_INIT_AND_UPDATE_AAD) {
            total_len = (pCase->aadlen - 16);
        } else {
            total_len = pCase->aadlen;
        }
        loops = ((0 != (total_len % 16U)) ? ((total_len >> 4U) + 1U) : (total_len >> 4U));
        for (i = 0; i < loops; i++) {
            WDT_FEED;
#ifdef AES_PROCESS_LATCH_RECOVER_SUPPORT
            if ((i == 0) || (i == 3)) {
                printf("[log]++aes_latch recover begin\n");
                LL_AES_process_status_latch(&pParas, &tmp_latch_buf);
                aes_128_gcm_demo_init(&pParas, AES_enCrypt);
                aes_128_gcm_demo_crypt_sign(&pParas);
                LL_AES_process_status_recover(&tmp_latch_buf, &pParas);
                printf("[log]++aes_latch recover finish\n");
            } else if (i == 1) {
                printf("[log]++aes_latch recover begin\n");
                LL_AES_process_status_latch(&pParas, &tmp_latch_buf);
                aes_128_gcm_demo_init(&pParas, AES_deCrypt);
                aes_128_gcm_demo_decrypt_verify(&pParas);
                LL_AES_process_status_recover(&tmp_latch_buf, &pParas);
                printf("[log]++aes_latch recover finish\n");
            }
#endif
            calcSize = ((total_len > 16U) ? 16U : total_len);
            total_len -= calcSize;

            if (aad_mode == AES_INIT_AND_UPDATE_AAD) {
                tmp_addr = (uint32_t)((uint8_t *)aad + 16U + i * 16U);
            } else {
                tmp_addr = (uint32_t)((uint8_t *)aad + i * 16U);
            }
            tmp_err = LL_AES_update_aad(&pParas, (const aesAAD_t *)tmp_addr, calcSize);
            TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);
        }
    }

    tmp_err = LL_AES_gmac(&pParas, (const PlainData_t *)aad, pCase->aadlen, CypherDatasIsTheLast);
    TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);
    (void)LL_AES_crypt_deinit();

    if (AES_enCrypt == encrypt_sel) {
        tmp_err = memcmp((const uint8_t *)tagSign, (const uint8_t *)tagVer, pCase->taglen);
        if (0 != tmp_err) {
            printf("[log]++tagSign\ttagVer\n");
            for (i = 0; i < (pCase->taglen >> 2U); i++) {
                printf("%08X %08X\n", tagSign[i], tagVer[i]);
            }
        }
        TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);
    } else {
        tmp_err = memcmp((const uint8_t *)tagVer, (const uint8_t *)tagSign, pCase->taglen);
        if (0 != tmp_err) {
            printf("[log]++tagSign\ttagVer\n");
            for (i = 0; i < (pCase->taglen >> 2U); i++) {
                printf("%08X %08X\n", tagSign[i], tagVer[i]);
            }
        }
        TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);
        TEST_ASSERT_EQUAL_UINT(AES_VERIFY_SUCCESS, verStat);
    }

    return tmp_err;
}

void aes_updateAAD_test(void) {
    uint32_t i;
    uint32_t tmp_err = SEA_SUCCESS;

    printf("***********AES Update AAD test Start*********\n");

#ifdef AES_GCM_TEST_ENABLE
    WDT_FEED;
    for (i = 0; i < AES_GCM_AAD_TESTTIMES; i++) {
        tmp_err = aes_gcm_testunit((SEA_AES_AAD_TESTLIST *)aes_aad_testlist[i], AES_enCrypt, AES_INIT_AAD_ONLY);
        TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);
        printf("------[log]Initialize AAD only, GCM encryption Case %d run Pass------\n", i);
    }
    printf("\tGCM encryption test finish.\n");
    printf("\n");
    WDT_FEED;
    for (i = 0; i < AES_GCM_AAD_TESTTIMES; i++) {
        tmp_err = aes_gcm_testunit((SEA_AES_AAD_TESTLIST *)aes_aad_testlist[i], AES_deCrypt, AES_INIT_AAD_ONLY);
        TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);
        printf("------[log]Initialize AAD only, GCM decryption Case %d run Pass------\n", i);
    }
    printf("\tGCM decryption test finish.\n");
    printf("\n");
    printf("\n");

    WDT_FEED;
    for (i = 0; i < AES_GCM_AAD_TESTTIMES; i++) {
        tmp_err = aes_gcm_testunit((SEA_AES_AAD_TESTLIST *)aes_aad_testlist[i], AES_enCrypt, AES_UPDATE_AAD_ONLY);
        TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);
        printf("------[log]Set AAD only, GCM encryption Case %d run Pass------\n", i);
    }
    printf("\tGCM encryption test finish.\n");
    printf("\n");
    WDT_FEED;
    for (i = 0; i < AES_GCM_AAD_TESTTIMES; i++) {
        tmp_err = aes_gcm_testunit((SEA_AES_AAD_TESTLIST *)aes_aad_testlist[i], AES_deCrypt, AES_UPDATE_AAD_ONLY);
        TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);
        printf("------[log]Set AAD only, GCM decryption Case %d run Pass------\n", i);
    }
    printf("\tGCM dncryption test finish.\n");
    printf("\n");
    printf("\n");

    WDT_FEED;
    for (i = 0; i < AES_GCM_AAD_TESTTIMES; i++) {
        tmp_err = aes_gcm_testunit((SEA_AES_AAD_TESTLIST *)aes_aad_testlist[i], AES_enCrypt, AES_INIT_AND_UPDATE_AAD);
        TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);
        printf("------[log]Initialize and set AAD, GCM encryption Case %d run Pass------\n", i);
    }
    printf("\tGCM encryption test finish.\n");
    printf("\n");
    WDT_FEED;
    for (i = 0; i < AES_GCM_AAD_TESTTIMES; i++) {
        tmp_err = aes_gcm_testunit((SEA_AES_AAD_TESTLIST *)aes_aad_testlist[i], AES_deCrypt, AES_INIT_AND_UPDATE_AAD);
        TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);
        printf("------[log]Initialize and set AAD, GCM decryption Case %d run Pass------\n", i);
    }
    printf("\tGCM dncryption test finish.\n");
    printf("\n");
    printf("\n");
#endif

#ifdef AES_GMAC_TEST_ENABLE
    WDT_FEED;
    for (i = AES_GCM_AAD_TESTTIMES; i < (AES_GMAC_AAD_TESTTIMES + AES_GCM_AAD_TESTTIMES); i++) {
        tmp_err = aes_gmac_testunit((SEA_AES_AAD_TESTLIST *)aes_aad_testlist[i], AES_enCrypt, AES_INIT_AAD_ONLY);
        TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);
        printf("------[log]Initialize AAD only, GMAC encryption Case %d run Pass------\n", (i - AES_GCM_AAD_TESTTIMES));
    }
    printf("\tGMAC encryption test finish.\n");
    printf("\n");
    WDT_FEED;
    for (i = AES_GCM_AAD_TESTTIMES; i < (AES_GMAC_AAD_TESTTIMES + AES_GCM_AAD_TESTTIMES); i++) {
        tmp_err = aes_gmac_testunit((SEA_AES_AAD_TESTLIST *)aes_aad_testlist[i], AES_deCrypt, AES_INIT_AAD_ONLY);
        TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);
        printf("------[log]Initialize AAD only, GMAC decryption Case %d run Pass------\n", (i - AES_GCM_AAD_TESTTIMES));
    }
    printf("\tGMAC dncryption test finish.\n");
    printf("\n");
    printf("\n");

    WDT_FEED;
    for (i = AES_GCM_AAD_TESTTIMES; i < (AES_GMAC_AAD_TESTTIMES + AES_GCM_AAD_TESTTIMES); i++) {
        tmp_err = aes_gmac_testunit((SEA_AES_AAD_TESTLIST *)aes_aad_testlist[i], AES_enCrypt, AES_UPDATE_AAD_ONLY);
        TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);
        printf("------[log]Set AAD only, GMAC encryption Case %d run Pass------\n", (i - AES_GCM_AAD_TESTTIMES));
    }
    printf("\tGMAC encryption test finish.\n");
    printf("\n");
    WDT_FEED;
    for (i = AES_GCM_AAD_TESTTIMES; i < (AES_GMAC_AAD_TESTTIMES + AES_GCM_AAD_TESTTIMES); i++) {
        tmp_err = aes_gmac_testunit((SEA_AES_AAD_TESTLIST *)aes_aad_testlist[i], AES_deCrypt, AES_UPDATE_AAD_ONLY);
        TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);
        printf("------[log]Set AAD only, GMAC decryption Case %d run Pass------\n", (i - AES_GCM_AAD_TESTTIMES));
    }
    printf("\tGMAC dncryption test finish.\n");
    printf("\n");
    printf("\n");

    WDT_FEED;
    for (i = AES_GCM_AAD_TESTTIMES; i < (AES_GMAC_AAD_TESTTIMES + AES_GCM_AAD_TESTTIMES); i++) {
        tmp_err = aes_gmac_testunit((SEA_AES_AAD_TESTLIST *)aes_aad_testlist[i], AES_enCrypt, AES_INIT_AND_UPDATE_AAD);
        TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);
        printf("------[log]Initialize and set AAD, GMAC encryption Case %d run Pass------\n", (i - AES_GCM_AAD_TESTTIMES));
    }
    printf("\tGMAC encryption test finish.\n");
    printf("\n");
    WDT_FEED;
    for (i = AES_GCM_AAD_TESTTIMES; i < (AES_GMAC_AAD_TESTTIMES + AES_GCM_AAD_TESTTIMES); i++) {
        tmp_err = aes_gmac_testunit((SEA_AES_AAD_TESTLIST *)aes_aad_testlist[i], AES_deCrypt, AES_INIT_AND_UPDATE_AAD);
        TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);
        printf("------[log]Initialize and set AAD, GMAC decryption Case %d run Pass------\n", (i - AES_GCM_AAD_TESTTIMES));
    }
    printf("\tGMAC dncryption test finish.\n");
    printf("\n");
#endif

    printf("***********AES Update AAD test End*********\n");
}
