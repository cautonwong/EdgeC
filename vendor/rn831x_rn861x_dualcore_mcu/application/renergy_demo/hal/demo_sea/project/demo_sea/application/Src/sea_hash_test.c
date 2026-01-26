#include "sea_test.h"

#define HASH_SHA1_TEST_SUPPORT
#define HASH_SHA224_TEST_SUPPORT
#define HASH_SHA256_TEST_SUPPORT
#define HASH_SHA384_TEST_SUPPORT
#define HASH_SHA512_TEST_SUPPORT

uint32_t hashOutSize(HASH_InitTypeDef *pParas) {
    uint32_t calc_data_len;

    if (pParas->bitWideMode == SEA_SHA_1) {
        calc_data_len = NOUT_SHA_1;
    } else if (pParas->bitWideMode == SEA_SHA_224) {
        calc_data_len = NOUT_SHA_224;
    } else if (pParas->bitWideMode == SEA_SHA_256) {
        calc_data_len = NOUT_SHA_256;
    } else if (pParas->bitWideMode == SEA_SHA_384) {
        calc_data_len = NOUT_SHA_384;
    } else if (pParas->bitWideMode == SEA_SHA_512) {
        calc_data_len = NOUT_SHA_512;
    } else {
        printf("hash mod error\n");
    }
    return calc_data_len * sizeof(uint32_t);
}
/*
*1  单次完成
    a 小于1个block，且DATA+FILL + ADD
        B 小于1个block，且DATA+FILL,  ZERO+ADD
        C 等于1个bllck
        D 大于1个block
*2  2单次完成
    A)block长度对齐
        B)block长度不对齐
*/

// SHA-160 SHA-192 SHA-256
// BLOCK_LEN_BIT = 512   ADD_BIT  64  ,448 - 8
/* -----------------------------SHA1 data start----------------------------- */
const uint8_t sha_160_requst_256[] = {"7CDFE331B718D70DD422817F5F09F7D0711478C6ED195BA75E154571BDDEBAA2"};
const uint8_t sha_160_requst_376[] = {"1B069A8EA9243308FCF9851EFC669FAB2000B01AEDAC2DA61508983342C6BAFEAC37A8CCD8"
                                      "4A2A9162846A6A3051E3"};
const uint8_t sha_160_requst_456[] = {"84840B02E331EBC1B57CB06654D375CEF51793AAE9350CF204171342EFE1E1F7C9FB819892"
                                      "4474037BDFC5135AAD42DFA28779E53E28F0051C"};
const uint8_t sha_160_requst_512[] = {"8E6F8AEFE3B34CC1A0EA988811E8C25601BC1FDD25266F3BE0038CEFE98577B6770491174F"
                                      "AE88C1D04DB450A7B6AAF669DB97225034AE1F87D99D6C751D0E54"};
const uint8_t sha_160_requst_1656[] = {"536F807B459B3BB30A332BA1C69C3655C01CD8A26CE55990E5D7C5B07AB84AE6F6E4868353"
                                       "5C0A9AB7DEEBE75AE1509AA2E9CF09BA66C4D9F3D3D75036F392F51A85821737D93E902692"
                                       "0930BF4D64BD28FBA30CAD646C79FCBE5D91EC312CC10FAA34BE9E799645F57D6E26E0C097"
                                       "5165C48163B4A19F4E442190CD1F340D94B8E9F1569EC982AA5E58AE4DB3B19A8D8B7CC6FA"
                                       "C6E5AB0001D2882023F49DA4CA21D77B4D23CE2CBF48485FA2A121CD42B4C2BDD8D7B69065"
                                       "3F7A6CF641F2A7B750EED47FDD414D3A9FD17C76103F"};
const uint8_t sha_160_requst_2048[] = {"3C1FA1136EA405CCA25F825B2177274C2EBE6997BFE690D492AD62793DCE144234EFDB1F72"
                                       "A84295548E8DBE1C2EE8AD5068125CE6180C183F14BCB039595B301E9F8A092577B4401B50"
                                       "B0CCFED0E94E5E9BD298FF32E1BEF8019727E85EE5045A8C8E067D46492200118FC7D70519"
                                       "D73CA40E1E5DB6BCACA507506F9AB551AB1887D118A725F36CBA304EBBDF87C6487031DD3A"
                                       "09FD8C2CA4362B70B304534FCC6F4F9E4E8A0919DF181CEAE260E2D1780373ED051D1B6725"
                                       "20A2B03ED285D71A311CC2583E4CE31FE2572464B24C46E76A4338DB6D81E931A7DC9A6D28"
                                       "FB363D809C2FA7C93BBB3B4E5A87064C0E02BA10D93A09C257B661C48C93FA87327A"};

const uint8_t sha_160_expected_256[] = {"5C38F8883A4FB5A274BB04B3625FCD7C276F41FC"};
const uint8_t sha_160_expected_376[] = {"F06F987BA1B958406C30F3889C8266BCE863E72B"};
const uint8_t sha_160_expected_456[] = {"40BA1BB7BE33C3E2D9B5E323EEE3F8FD4731201E"};
const uint8_t sha_160_expected_512[] = {"86F47FAEA11B59B547BB403E15D2B61DDFCD8A69"};
const uint8_t sha_160_expected_1656[] = {"AADC5B9ECC9B5A0EC22E49F675FD4E0A860895BD"};
const uint8_t sha_160_expected_2048[] = {"959EDCFCD8508824E9166322D47F158FB4023531"};

const HASH_InitTypeDef sha_160_configer = {SEA_SHA_1, HASH_GENERAL_MODE_EN, {0, 0}};

const SEA_HASH_TESTLIST sha_160_test_00 = {(uint32_t *)sha_160_requst_256, sizeof(sha_160_requst_256) >> 1, (uint32_t *)sha_160_expected_256};
const SEA_HASH_TESTLIST sha_160_test_01 = {(uint32_t *)sha_160_requst_376, sizeof(sha_160_requst_376) >> 1, (uint32_t *)sha_160_expected_376};
const SEA_HASH_TESTLIST sha_160_test_02 = {(uint32_t *)sha_160_requst_456, sizeof(sha_160_requst_456) >> 1, (uint32_t *)sha_160_expected_456};
const SEA_HASH_TESTLIST sha_160_test_03 = {(uint32_t *)sha_160_requst_512, sizeof(sha_160_requst_512) >> 1, (uint32_t *)sha_160_expected_512};
const SEA_HASH_TESTLIST sha_160_test_04 = {(uint32_t *)sha_160_requst_1656, sizeof(sha_160_requst_1656) >> 1, (uint32_t *)sha_160_expected_1656};
const SEA_HASH_TESTLIST sha_160_test_05 = {(uint32_t *)sha_160_requst_2048, sizeof(sha_160_requst_2048) >> 1, (uint32_t *)sha_160_expected_2048};

const SEA_HASH_TESTLIST *sha_160_testlist[] = {(SEA_HASH_TESTLIST *)&sha_160_test_00, (SEA_HASH_TESTLIST *)&sha_160_test_01, (SEA_HASH_TESTLIST *)&sha_160_test_02,
                                               (SEA_HASH_TESTLIST *)&sha_160_test_03, (SEA_HASH_TESTLIST *)&sha_160_test_04, (SEA_HASH_TESTLIST *)&sha_160_test_05};

/* ----------------------------SHA224 data start---------------------------- */
const uint8_t sha_224_requst_00[] = {"E7DB748E16BB1E123327F0E9331A4C808AD7232D1FEDC1E77842B6A8E1DEE205"};
const uint8_t sha_224_requst_01[] = {"66279BA0163AF6065E1D53D379E7D35508826AC257909A834B874428CFF5495EB4D5BD20EC"
                                     "70608B38386CDE0DFD3ED1"};
const uint8_t sha_224_requst_02[] = {"1BA5BD72A2CA6DED1E0E304BE400BF855BC3A32DAF53409C2BC05B05F18CC49222DF799E6C"
                                     "ADD20259FDC338DC518FD1D5CBF80387D8719444"};
const uint8_t sha_224_requst_03[] = {"595C6C0493177CD86D935E497A6382A013107F6DA09121F714ECBFCAF390A167F2C0FE64FD"
                                     "101571E1EEACC0F0ED92A5D7952DB7A28C8757C16A14A9885DC61A"};
const uint8_t sha_224_requst_04[] = {"A0BF85A9FBF5BA5F77FCB16B92E11E9384E4B7B6C0FC1EDC0C68288048F17FF1494AC15D8D"
                                     "638AA4CD0AACCF50A304D88B2F472E5452E2611D120D8699876923CC010E97B9AAC2CB130C"
                                     "09A35E3BEF19EEC107B2154CF358E17A08502E9EB0D7AD0A0B3FBCF29A93E15C7A47AAC4DC"
                                     "25633B244A5E172DD841CA8A533DA4278D54B4673A615255120C66C9422434226CCE46F455"
                                     "2B048B15456D911105A6F9AA9F9F7CCD30AB6B5791504FEA2C7345D2DC8FF9374FC62CD082"
                                     "98E6CDA9E6625B50F5B5"};
const uint8_t sha_224_requst_05[] = {"49C2D352EC8C8DFF392BCE732791AD7A144E8BCED32B565CADC7B1DF69B0E297FAC4F43AD9"
                                     "CA56F98E5B1A1411435B25A1DDE283954BDE3DFDAC1C42851B5A5340DFB783B64F58F6D121"
                                     "19B1685309CA0936FBC3042376B247AA8A31C0271AA70D82681A0E655B9DA1E6A60CFD26E8"
                                     "87DEDD699FC368D6A8FAC5E912ECF9DC772D17D850DBE7A773D6BE2BDACC717731BD772B9D"
                                     "9F48BFE5D34302B5D9325CCDC0727E7557A5FE097A09E8824DA9ECFD7BC34548C79DEAAB8C"
                                     "2FF6D9F1227DBE417984B86EF3F402C0F23A8AF44B14CA1DF4D857F19D46452BEC684BA8BE"
                                     "FBB593DCCE62740AF1541A36E60EE21598F47A9F98EDD8119B10DBF8C25F470E39A7001C9F"
                                     "8C408F69B5E1B0688CAE223DED4CB05AE265FCD133680EF1DEBFD20483C395301F03E5A4D4"
                                     "81FEE9A945031FCDD682324C91FC3C303CA371E564EBB3CA5AAD90CDEF14707FFCEBDFA12D"
                                     "0ED6DAF455C9739C5F30536220347364A1C0609F16B71CDE1322A0F6CFABC2509E507017D6"
                                     "86244C53E012C96EE112B4664EA8AE2F40"};

const uint8_t sha_224_expected_00[] = {"B65899CC4F3E02EF1C21A16A6E02713875D704CD72F677A12A39453F"};
const uint8_t sha_224_expected_01[] = {"929F4E875A5FB7EA6E432DA2C9332CD5A17786CD993CBD7ED677EF7D"};
const uint8_t sha_224_expected_02[] = {"08229CE6328D8E10C88284B69B7CEF0A817E7545D24AED28A06CDF9A"};
const uint8_t sha_224_expected_03[] = {"B56C8380BD0FA70E6E1D2D6EE1E2245F5635089EAD8C16C5476FBB9E"};
const uint8_t sha_224_expected_04[] = {"5A38C3335A8DEB75233F86F6D9C6585BC8550FF39DC682DEA82FBFE0"};
const uint8_t sha_224_expected_05[] = {"8302D4C78A7DFEEDF3859147F2F397E0B2CC72EDEB65DCB77FD554E3"};

const HASH_InitTypeDef sha_224_configer = {SEA_SHA_224, HASH_GENERAL_MODE_EN, {0, 0}};

const SEA_HASH_TESTLIST sha_224_test_00 = {(uint32_t *)sha_224_requst_00, sizeof(sha_224_requst_00) >> 1, (uint32_t *)sha_224_expected_00};
const SEA_HASH_TESTLIST sha_224_test_01 = {(uint32_t *)sha_224_requst_01, sizeof(sha_224_requst_01) >> 1, (uint32_t *)sha_224_expected_01};
const SEA_HASH_TESTLIST sha_224_test_02 = {(uint32_t *)sha_224_requst_02, sizeof(sha_224_requst_02) >> 1, (uint32_t *)sha_224_expected_02};
const SEA_HASH_TESTLIST sha_224_test_03 = {(uint32_t *)sha_224_requst_03, sizeof(sha_224_requst_03) >> 1, (uint32_t *)sha_224_expected_03};
const SEA_HASH_TESTLIST sha_224_test_04 = {(uint32_t *)sha_224_requst_04, sizeof(sha_224_requst_04) >> 1, (uint32_t *)sha_224_expected_04};
const SEA_HASH_TESTLIST sha_224_test_05 = {(uint32_t *)sha_224_requst_05, sizeof(sha_224_requst_05) >> 1, (uint32_t *)sha_224_expected_05};

const SEA_HASH_TESTLIST *sha_224_testlist[] = {(SEA_HASH_TESTLIST *)&sha_224_test_00, (SEA_HASH_TESTLIST *)&sha_224_test_01, (SEA_HASH_TESTLIST *)&sha_224_test_02,
                                               (SEA_HASH_TESTLIST *)&sha_224_test_03, (SEA_HASH_TESTLIST *)&sha_224_test_04, (SEA_HASH_TESTLIST *)&sha_224_test_05};

/* ----------------------------SHA256 data start---------------------------- */
const uint8_t sha_256_requst_24[] = {"EC7EEF"};
const uint8_t sha_256_requst_256[] = {"219719A8AD576BA896B6FF3101AAB8CE51107DF878D5197C88AB5E44B8CC3833"};
const uint8_t sha_256_requst_376[] = {"C9ED8870E38BA454251F06A11FEA558E7601F8E14A0FF008C535E9FFDDF77B6F4B03D324A9"
                                      "77598E8FF3D3D3626D77"};
const uint8_t sha_256_requst_456[] = {"210262CA072B47EBD49799EA75CB89265486542485B822EA6B54A8D71E65035E5DBEB9A850"
                                      "2EF8C0DF629CAB632DE171DF7C1E548A163DDAAA"};
const uint8_t sha_256_requst_512[] = {"C0DBC31692948BA3996FDD6965D9AB5318BF11254D267D6671CDB6F13F113E2466A321211E"
                                      "FDDE7CE5C5ABCEC6E8A42B4279A5127852E8E1FDE1137010706945"};
const uint8_t sha_256_requst_1904[] = {"83AC8CDBDC4870DE2909111D8CE2A9CF0F480F9AD746BA30A06E8D0776980D9B25D8F9A4AB"
                                       "F3C5D18298AAA891E9C42021E8BB09ED7ED67C5038CEA21AF377D4B1A3065002CAB8CA51FC"
                                       "19AEC30B686E10A7FBED6CBE3E2C28FB6E64BD17CD9C476DB4FA58D4C71CD628C82BAAFF78"
                                       "BF9C8CB9E890BE41659D5F25CAE68FF31A28C4AD3BCB04FBC98F8BACAE24DC8B4EC655F3B1"
                                       "31F661F4A97FFF549E57A782D6ED08364D3F15903EEBDEF1B3EAE59CB4C4228E12CA0730FB"
                                       "FB7AE0953AB46E7C60980A783F12E8E3057B38265CF02360948E53C624E36FF781766027F8"
                                       "96BA23054EA40E45304217E8B2F539FE"};
const uint8_t sha_256_requst_8192[] = {"7A2761F72D15FFA862D352A6C452783FA966A564BEC5B00D5B9E575C152878287D02D2FA21"
                                       "CE2B49ED81D80041450765E849EEEE2B3A964D2183C5595FEC2A3BD394ECCCB860899855D0"
                                       "9F5512D29121F7B6493F634CBC69CE8B5AAA67801A8CD7EBD2D267FF1C3189523BE8A465D8"
                                       "7C2E2DD97A790126A66754CBD49112EB90B943BAEB4EBF4ED130C2408AF224BD9A60ECBD52"
                                       "10D4D091D62191B6D04CF4CADC2E0C62FF3868EB7FEA4E4993FDC7F80A9B2185DCC7BEC6A6"
                                       "BA23B7AACEBB7E0CD488BA1CC83681719A1ACF9B764BFAD83C858CCB009E64E9292B35B9DA"
                                       "8599BAE9C0DFF8A0348F28EC867B2CA7F0D1EFDD9130687568CDB3805B51585E6DF8154677"
                                       "8B473BD76A6CED0133CE52E4447B839327D29BE03175081ACF0AA07D292054CD11EC6B5482"
                                       "0463CB49A7DED5CE5066F2D5878D4CE35863F79CBEDCCD3736A911A32034AB6D581BB41E7E"
                                       "38956CA5D8ACD8D8B3D232F8EE4697D9AD9EB98249823C4A7AA7FC3ECA895ABF44D19EFBD0"
                                       "4A5C964F80B8E55C1EC731E938E27674634FD7903DEF344227586CB88E16B57781128F142E"
                                       "E59859CC876ADC3F6623F5134E5B86167CE2DA70C4351811B2E488A371505BD789C2BED6ED"
                                       "0C20C9A5075D717E6A729F59E8D16D679DF1A46E46D9A8338147C394737DC0FAE0FE4E5041"
                                       "0EB5D987588B8CBC24B814EF4B0783B9B6EC5A710BB3BD36F8D6F5FE5B7EA1E55DB2A5745E"
                                       "253234193E3404FA8C4A73BF565EA033AEF4B32A37AC8BBFA5FB59BD0E7C957B4210158C8E"
                                       "1FFF67E76EA562B5A456D85EF4D94720B76E3BBFC3383E9CEC6931AC2964AD37FE28883CD4"
                                       "E1A6E74B82DE39F6A977AD41A4A7BD4D8BB7A73B3BC9158CAD38838FCE086C8789C55E2C88"
                                       "4B7AE079D8B57EBE5962E73D0607940FC59A40D36F96FDBFB2CE5EF66C9A068EFB53924BB7"
                                       "00D609E8FC433542429D6762F323CCED28DBDEC6987AAEBE67BB9C972980A36EF703E9662D"
                                       "FB4B74C3572C7C1D5D27379EB87BF60043BBE93C36E448B8F8996F1A84C01A5B6BA6A3D2DF"
                                       "095363EBF54B035490C3EFBDF2B71E44D1D39E1D53CCAA906712A82A8C2F7C6AB4F3FAD92E"
                                       "0E0A83650BC544FF81966844E79A220E86D2042C92F9FFCE68605608EE3F8920632C6CF5AC"
                                       "A426D1D72261117792EC858459F37C1B3C5B7A002F0931961B22E3A8E0D7D41FA6677A4F1F"
                                       "69ED74FFFE7292ED24112331003A008B9C141E75F04010D61175057E72A86A17AE1B775B06"
                                       "9C4428A7875C33B9B955DC53924FD291FD3AE1497B9C429A39748044DDEB5617541EEBE6B0"
                                       "732B225951ADB79FB7E629AFF499718FAF25882AD81AB27F8C0B660D7C0A7978818597AC70"
                                       "F9A0246C73532600D1E95263945E976467350AD79253F2D124130A0D2980DDEBD5230EBCEC"
                                       "910E248A11E0FE38328F1586A20162838294085D10FE8177B0"};
const uint8_t sha_256_requst_9728[] = {"910D172CC05D587B6D8B5BE4FC0C54A1EABB70C8D2BC46AA050394C3BB9CD14B37BAB1BF59"
                                       "E341FA270FF88A54AC6465B334E070C79BED42CD710F02587F8B6615B090617208D6CB2C38"
                                       "63C0FF131C1505EC434602E7FBC81B23C730AE6D328BCFF79D71C16A52F77BE258C3630966"
                                       "94A1BE6F8B0A5A5DB32152CBC4FF51AE84FCC7CA66A8BED474774EA0609B1C1AE7E8C7815C"
                                       "F12E52995D462F96DEC9451C43F3E07EF04F5C231B60B95DF4BBF950E8AA8E456DF177DC45"
                                       "BBB504E1EFD531AC951B956D0291760CEF4E680B9855134A1185B4FBF2B15E884D8081C568"
                                       "1269C9C5F649CE44616D5E89371F50FF29C63D7B6DF4599FF6A2DFEAD9CF70EFFF440E9E68"
                                       "355F928EB705DE53F1949149621CF171C5AC200832FA56DB5DFC674CB17ACCB0996146B8AE"
                                       "41191561D1F0F9BB47D536BDA17A10BF37CA7B1840CFD11A4025E28E4D913753AF1240BDE0"
                                       "40FD756E1C2CCD828CD75D5AFD2A999967C181D7F5A1CE6667D874A3B84A78D76BBD626415"
                                       "7F0151A910E1F448F52505DB0BF98B18289DA2618AED1E7D77926ECBA550232F75A5604C9A"
                                       "FF924330F155F6F11A4AB8437DBBE092D9BAE04DEBA8A325243A4A1BF7F656B1CE96E37D34"
                                       "3B45CE6C681F3593EFDF9A3E266CFB121B30E310786B3623625A3A804B335D561FB946B5F2"
                                       "681B8CB39E8F706E7D5131D6FC02F3399CEE81FAC1D584ACE9317F5009BC789DD20F1D241D"
                                       "AD66EC30E537DFE7697CB2AD5FDC31E48702863FA960057CA153B530184079B4A199781F65"
                                       "82C35BC4554936FCF15D72A9B61288D1C6B6268652B2A892A33CB3B3AE9FBC4EDA9A998A57"
                                       "065F878BCF54FC48D455F32C634155C7FA001B4EB2CAB5D1C2CB4559BA7D0B05B534070CED"
                                       "8152CEF6456EF284757AE5160858A69F408C0D6C740D2222F38D424778A2DD266CCF53A6BB"
                                       "8F3FD82145D0ED684C47BF537D57122E78BDD4F98A85844614CA45674E625017F91D989E3B"
                                       "E432E21BAC7912885B42E74474CE206A9FD827DD140678B20815C55947AEC03F57A247DD3F"
                                       "9962AD625A1A42F8706328A95FA8DDD7F5DE2C0F22B40F2674E37684DFB5D8F7C9CA48B6FF"
                                       "D3F23287D4BD25ECCE2A25451E0C7B3FA709E119AC364D4DCACC2D8915788ACD23F602596B"
                                       "B72C9529D3AB30C34FACE2610C425416D9F5D64C9E3E01534EBF795BE994CBEFDC4BE95EF7"
                                       "F3A20C9AEC08CE80D7D4F1E5B6B7C2F136B7A08A8D1C75B7CAB0587A6339E13439CD91B7C7"
                                       "8255DF8BE06B35312699EC4652152D1D98C4855AD41D41E85961430A220B50A408434BE4BE"
                                       "2252A2D08BE7F140D4C47CEC57451D35C00BCE3A9EAE330D05BC0E8B2D1A17911974DDE2BA"
                                       "1ADFEB7C8607945010DF19DBD1225BDDFE3300345CEA6BD36EC7DC59CDFC365FF806E266AC"
                                       "39D1AE686AF028E1F9F4F6705F16AB3DDAC1FC4698B658B6FDBB0108FBD5C518CC3C15DE3D"
                                       "0F1EE7B16C241F925688FE3E5ACECCFB32E94FB0EC7789D8D94D1E1388C7E714EA97A98EF8"
                                       "79810508A37444D9E149F3D9A193242AC9D5B8ADDF2AED2A2735CBCDD720A4C2A59CC60E79"
                                       "79C23036B8043B483BBE92B5EC0B3C6B34943F8F7E8265A57B91E6853C145F26D916D41124"
                                       "E1746A7DA9A99A7148690729FEBAFCECE7904ED354182C6FB9F1E55904FD5512656AEE912C"
                                       "770ED038FA93BE553E2EF9B92AD76CE7768ED00596F5B77AD36BDC285D682DB3"};
const uint8_t sha_256_requst_1024[] = {"2CAD21BDF7586491F44989AC62DD06596A725DF0787612921BE7686BD2BEFED898526F1A59"
                                       "32272141EAA053B5CDDBEDC81318BF460A2E9BC87BEF1E7AA67C959D3E7A3ECBCF6D8331A3"
                                       "FAA712DA0D0B05E2114E539B1712903BE9C6B558C53C91B22EC3D9E1D0DA626B5B5D1CBED6"
                                       "B142A57740E30E47CB02531F9CA9CFEF8C"};

const uint8_t sha_256_expected_24[] = {"F434DE2963640D0A963F46D968AA880131DB9820FBC9FE4F0E0AFECD85F8B57A"};
const uint8_t sha_256_expected_256[] = {"F35EB51A581C568D63D0583D7BFBC2DAE38BB85DB67C6209CE489427C8B0873E"};
const uint8_t sha_256_expected_376[] = {"62F485225CEFE6E856CBE94E43ED948212D5FCC2421D5D00C0F2A318D818AA74"};
const uint8_t sha_256_expected_456[] = {"6FFA306E8A5BCE8FE5FB0E5BF0B768B5216E75C5765A5B2D5E023921B77540E6"};
const uint8_t sha_256_expected_512[] = {"AC25F5C0B478E5C7C53CD6879D0E510425693D651686D076068C773DAB9E6FB9"};
const uint8_t sha_256_expected_1904[] = {"585290196988D941E099FB73071CEF8BAEED9CB62DB50B3E48ECB6E232F0BD59"};
const uint8_t sha_256_expected_8192[] = {"911F1B06A76682E23AB39179AA293600CB99271AB4C425B4EA5CD67DA2BCA675"};
const uint8_t sha_256_expected_9728[] = {"BA62C8F48C17A444D2675696ECC01EA5F6D1EEE67186BE7BC780690271A07076"};
const uint8_t sha_256_expected_1024[] = {"7D8CCCAE0A2D52FF7EC3E5756E327D0076298127B61B1FBC101D4011C679B67F"};

const HASH_InitTypeDef sha_256_configer = {SEA_SHA_256, HASH_GENERAL_MODE_EN, {0, 0}};

const SEA_HASH_TESTLIST sha_256_test_00 = {(uint32_t *)sha_256_requst_24, sizeof(sha_256_requst_24) >> 1, (uint32_t *)sha_256_expected_24};
const SEA_HASH_TESTLIST sha_256_test_01 = {(uint32_t *)sha_256_requst_256, sizeof(sha_256_requst_256) >> 1, (uint32_t *)sha_256_expected_256};
const SEA_HASH_TESTLIST sha_256_test_02 = {(uint32_t *)sha_256_requst_376, sizeof(sha_256_requst_376) >> 1, (uint32_t *)sha_256_expected_376};
const SEA_HASH_TESTLIST sha_256_test_03 = {(uint32_t *)sha_256_requst_456, sizeof(sha_256_requst_456) >> 1, (uint32_t *)sha_256_expected_456};
const SEA_HASH_TESTLIST sha_256_test_04 = {(uint32_t *)sha_256_requst_512, sizeof(sha_256_requst_512) >> 1, (uint32_t *)sha_256_expected_512};
const SEA_HASH_TESTLIST sha_256_test_05 = {(uint32_t *)sha_256_requst_1024, sizeof(sha_256_requst_1024) >> 1, (uint32_t *)sha_256_expected_1024};
const SEA_HASH_TESTLIST sha_256_test_06 = {(uint32_t *)sha_256_requst_1904, sizeof(sha_256_requst_1904) >> 1, (uint32_t *)sha_256_expected_1904};
const SEA_HASH_TESTLIST sha_256_test_07 = {(uint32_t *)sha_256_requst_8192, sizeof(sha_256_requst_8192) >> 1, (uint32_t *)sha_256_expected_8192};
const SEA_HASH_TESTLIST sha_256_test_08 = {(uint32_t *)sha_256_requst_9728, sizeof(sha_256_requst_9728) >> 1, (uint32_t *)sha_256_expected_9728};

const SEA_HASH_TESTLIST *sha_256_testlist[] = {(SEA_HASH_TESTLIST *)&sha_256_test_00, (SEA_HASH_TESTLIST *)&sha_256_test_01, (SEA_HASH_TESTLIST *)&sha_256_test_02,
                                               (SEA_HASH_TESTLIST *)&sha_256_test_03, (SEA_HASH_TESTLIST *)&sha_256_test_04, (SEA_HASH_TESTLIST *)&sha_256_test_05,
                                               (SEA_HASH_TESTLIST *)&sha_256_test_06, (SEA_HASH_TESTLIST *)&sha_256_test_07, (SEA_HASH_TESTLIST *)&sha_256_test_08};

// SHA-384  BLOCK_LEN_BIT = 1024  ADD_BIT  128 ,960 - 8
/* ----------------------------SHA256 data start---------------------------- */
const uint8_t sha_384_requst_384[] = {"E9A9CE8852E2ED71DEC73580D60C18777E91A44F0E07674ED9E827DB5A42A816C5EE6DEA4F"
                                      "59F85AC9982E34E4D1B241"};
const uint8_t sha_384_requst_512[] = {"3C0C78F26C76F75263F99892EDB99658BA7D8D743776E1481BC0AB65799A8FB7005480AA9D"
                                      "7D72DD32A619C0B84EBBA6455F911C4E9508BDB7166FDE5429F87D"};
const uint8_t sha_384_requst_968[] = {"5FD16ECEE19DD90257EBD2875BFBAEB51096BB81D577226B100F43401603095762C7999C38"
                                      "0FC6910E5AE3088A70016AA26FF624A5DB72F86DA6DDA82EE75D3AF45DF85D65502407BA61"
                                      "5D48708F11EDB87128733EFDB45B24FFEC5C1681F26BA353989C5EB9DCCAE235215A3797F8"
                                      "5A03F32A2052008D8CF9"};
const uint8_t sha_384_requst_1024[] = {"11A45A4BC4CC44F7DD443CA4BEA8C32A2BA332034E3F46777028F72C64CE5A6A5EC40A0784"
                                       "98B4A92F921B1407F860E131B7F1B66F639B1BABDEE379757489F858449D1E89F5E21D59A2"
                                       "67E95BF15654A16DA1EEC6B2AEFBE394FC66B50E4B977B76065E4D014ED3A5B8691D5F7FB0"
                                       "A8459987C10B056F5FA2EA20D8C887D6C2"};
const uint8_t sha_384_requst_1344[] = {"747E2DB8DC20DF0C7320A15BF8B5657CDC9100AEC7D1D278EC69C35550FF8905ED0CF6FDEE"
                                       "8E3D156658C039FDCE4B88FEB4734C6268AB6B7C1B461A36466533599F29443D64304FDE63"
                                       "17723C4E933283F99BB633E3A4165034611FDE1C787DCBFD889100CF819A6AA72F077FFC9F"
                                       "DD3A2436AEC43130E1302A13BFFE9BF60D6A966EDC2CCE57BAA860E764C5D54164F460BAD4"
                                       "29E2143AD9B460588929FB520BF4DCC060933EF5"};
const uint8_t sha_384_requst_2048[] = {"F6CBB6596DDAEF0F72282165330199D8B7DF90E2383BD2DCF37EF50A70CDFB6A50A937E055"
                                       "F4F4BF70FDD400B45756278502E46357F9F72DAC0C4E61995F0D4629D5BD8A281FDA9C42E8"
                                       "DB30697EE941E08F6A7E1324400D335F002F9037A74B2D3D7F9D9836B7F080F78897062B8D"
                                       "AC3CDE9941AB3B83A97255C1DF0F5B94CA7FE15BF72B252079D116BA97CEB262F43F0F8B24"
                                       "02272D8834ABC2B8B088063A8F564A691FCDBCC7039B68D8C4B854350E716716952320CA4B"
                                       "7BC4E7EC241439EF7E4F22E87166A726214823F17804F892AFDCC4C1F5B9DE7832DA8674E2"
                                       "E84BBE5951C8B1E6E4EB36290C4F0299FC260139C9D570D39240470108CC5C9CD132"};
const uint8_t sha_384_requst_3968[] = {"6AA81CB092076DF680B26CEDDB37E6128A152BD2884E33BD72788F3B9FC0114B7616F51F01"
                                       "53F6A1268335B96C0293CAFC3A6F9129165560C71E0388A9937AB56BD9F06E02E24CF9A654"
                                       "E9DEAEFADB760038CE19757D256685F8024E4D9DF60F61E4B448789302CB54E40E4E7DDB58"
                                       "C96C51259FA3DB991F21690204ADA0DEB5A366B907C448C726D872F50F9C47CF87FCB0EF2B"
                                       "5299761BEC789040BADA3D49F4AB954C62557A5C7D783919F78598B2A9BCDFEBEAD26138B3"
                                       "1C5D3EA5449E118BED9535F97844DC9BDAFEB3D2293849B7358458ECC83689E08FC5A73215"
                                       "377B2C0B432FCF50E82D15CCB4215C42C879C49054DCC0CA1CCF1C6485E2A65AA84FFB5BF6"
                                       "9AA2FCDA0B124423089853EE66716B01DB6532D622A6E62BA95BFA7239D44C744AA83AE4C5"
                                       "6BB90E97EDC2B5480EE1DC18937D9C6FA59ED487C145C520DB10CB4D9BFC474E84955901A2"
                                       "7CC07A6EEDEF201B4D4AA09A1047F088695BC4BAB013AE3D2335FD02F766DE1E367FEC8F5D"
                                       "F11D0A7FD1EFBF0971D1FA4A46EB747737226950D75F0BFAA0EC3D09AA0BE1413ACE54CF3C"
                                       "F2D37CFE158341F8A2E4A3E8C842612ECA2B46ED4772A46CBD1FF512F2ED2A8F5C407A8397"
                                       "1A03A5F8421291A9C7C901F43427F9E32C7D6C57141819293A3AB5764DAF92435662F12529"
                                       "19C5DEFC80D6381E4D2CA3A1DC0BD2"};

const uint8_t sha_384_expected_384[] = {"4CC18D34EDBA4C024133165B045C8C5AE62ED7D1CCC42DD1FAE7566B8D1EFEDF04E733F1A2"
                                        "D5E55D048C89181526A03D"};
const uint8_t sha_384_expected_512[] = {"A770AA20D87F886723AB0CF0C95E98306BEF518C8EAD97CA2225D8B33F67DB4CFE94A18332"
                                        "2848ECA23BC6F998CCE333"};
const uint8_t sha_384_expected_968[] = {"B8B2C08B1F82BABFD9E9CA7597171B3B3611774A37E9B67DA2C6A55C5DA5FCF3CD6B4DC81C"
                                        "3E3AF87391AFB0E56A2E8C"};
const uint8_t sha_384_expected_1024[] = {"1854992341D29C88344287522598258455011DB916BD5BAEF12E4DCC55B18698F51F682957"
                                         "C056C5DEC823B735C2AF8D"};
const uint8_t sha_384_expected_1344[] = {"1D168F23916CE39BD5E47DAADD34CCAEB52F2E4F617DB0A16750A79A0AE05A70870ECF9B2F"
                                         "819AFD517B440D2FC395E0"};
const uint8_t sha_384_expected_2048[] = {"EE4730D2FB69FAFA1FC099633F49B93128BF1D1D086EF7FC45FC82A4F80973BB07A23FF812"
                                         "CBA4922ECDE9D004EBA152"};
const uint8_t sha_384_expected_3968[] = {"60F38AADB1BDBFCB26147D8C394E2DC055C7B9DB773B394E4E8D65CA5DAB235CE0E2A9749E"
                                         "52C29BDE2067EA5659A59F"};

const HASH_InitTypeDef sha_384_configer = {SEA_SHA_384, HASH_GENERAL_MODE_EN, {0, 0}};

const SEA_HASH_TESTLIST sha_384_test_00 = {(uint32_t *)sha_384_requst_384, sizeof(sha_384_requst_384) >> 1, (uint32_t *)sha_384_expected_384};
const SEA_HASH_TESTLIST sha_384_test_01 = {(uint32_t *)sha_384_requst_512, sizeof(sha_384_requst_512) >> 1, (uint32_t *)sha_384_expected_512};
const SEA_HASH_TESTLIST sha_384_test_02 = {(uint32_t *)sha_384_requst_968, sizeof(sha_384_requst_968) >> 1, (uint32_t *)sha_384_expected_968};
const SEA_HASH_TESTLIST sha_384_test_03 = {(uint32_t *)sha_384_requst_1024, sizeof(sha_384_requst_1024) >> 1, (uint32_t *)sha_384_expected_1024};
const SEA_HASH_TESTLIST sha_384_test_04 = {(uint32_t *)sha_384_requst_1344, sizeof(sha_384_requst_1344) >> 1, (uint32_t *)sha_384_expected_1344};
const SEA_HASH_TESTLIST sha_384_test_05 = {(uint32_t *)sha_384_requst_2048, sizeof(sha_384_requst_2048) >> 1, (uint32_t *)sha_384_expected_2048};
const SEA_HASH_TESTLIST sha_384_test_06 = {(uint32_t *)sha_384_requst_3968, sizeof(sha_384_requst_3968) >> 1, (uint32_t *)sha_384_expected_3968};

const SEA_HASH_TESTLIST *sha_384_testlist[] = {(SEA_HASH_TESTLIST *)&sha_384_test_00, (SEA_HASH_TESTLIST *)&sha_384_test_01, (SEA_HASH_TESTLIST *)&sha_384_test_02,
                                               (SEA_HASH_TESTLIST *)&sha_384_test_03, (SEA_HASH_TESTLIST *)&sha_384_test_04, (SEA_HASH_TESTLIST *)&sha_384_test_05,
                                               (SEA_HASH_TESTLIST *)&sha_384_test_06};

/* ----------------------------SHA512 data start---------------------------- */
const uint8_t sha_512_requst_00[] = {"82BE95BD6E77A0DB32559046738FD295070617D0C2FC6010D6DBD797A7F202898EACB6FC8F"
                                     "F1BF76B47185F114FA6E8C"};
const uint8_t sha_512_requst_01[] = {"860316D19402FC68DB908429BB9C2FCFC4E2F2F2FD2956CCCA5A74A396C16895BE19C3EB2F"
                                     "35E7274C86E05EAED61FC54FA696958D39B24AEF46067B2949EEBB"};
const uint8_t sha_512_requst_02[] = {"224AFF42F980D3510949F2891CE97FC9F4A5E230FCE714B481D8FD6EB9209852379CF9A913"
                                     "F6C75DD985EC2266250A8CB48B2A1DF9A2E21BF31238C0DEADD7F66C59AF277634541FC2DC"
                                     "D2DF0208C7BA98DB1C901605ABE6C36FDD67158A3AAC65EF2A2DDFA368D01B4122A589A68B"
                                     "D17A4E4B18593CE69787"};
const uint8_t sha_512_requst_03[] = {"53F3D419253C015E9C81C8548AD14E3878C685842DAEEA4CDC4DFF66458B9DE1875840A4CC"
                                     "F270386D13BF08347C9357EF97CD1890AD369497A10C96756E715B89FCD01E283ED045DF16"
                                     "31634281EDE8E374D5B7E726DE587769E29D37504F9F770289226477CCC44EB43F6F06ADFD"
                                     "793E44E6C40EE85E121E3595F4021EF91E"};
const uint8_t sha_512_requst_04[] = {"9184E1F44B2373E1B3053CF7022DF6BDCEC6F9210ED8E259E32D96C82DEFB4C6E940036EBB"
                                     "B5D4E29C9B4B1FAA9909F6FC032FA92E1D2C95F29DAA2005107CFE24E7900C24F2C0FD6F27"
                                     "3E3B6DD4F68D4D9CD8AAEED8AD7A4D28607CB34309DBD7B9F3A1275692D5AE1D81DC977ABE"
                                     "53195D640FEB54B81788B763B7C76F1BF103"};
const uint8_t sha_512_requst_05[] = {"E0561ACCD3D09984C6A9A167259E88031EDBFD2B9CAC2CA3AD68823C190EBE9EA3E2D99D5C"
                                     "C9E128EA42B29E89AFCD277969908A79F605BAF47796B5F883C778472B17F2C6026DAE6677"
                                     "22A266211B431D7AC0E08706C9BAD5A0A0CA78479F0CE419C0A5F3EBA23A49179143B1DA5A"
                                     "94410EFC9890AB9207D00ADD683CB7C5F7BAD47CC1B65D511A13518C190E99C5B208DC813D"
                                     "3F5AE161DA76024505B58BD6309FDD778D6271C24C3CF2710D6C7ED24FE21DC78DF697974B"
                                     "092427209E5605C299858C6C159526A40E34D6F12D6AFE9D22E35CBA4E70B62C498C228916"
                                     "D18012E49E776B1CF4333FD816FDACC3E1D82CAD3DF11E68E96111F1A86148D4FEBA0CE199"
                                     "D30596ABDB1402C5A4B081A7D5AAD4F787D0DE62ACB7A40E17F70935047DC67E54988CAC23"
                                     "350ECF8015A1062008A06A7DD87C808B36325FB1BAE0E9E483DA46B6A1B0751552D0045169"
                                     "98DE1656C9F7E1EDB8DEC37C20040EA1FEF3395F3919F3B75CE900815735B73FFDAB3A4000"
                                     "B0AE19491D9CCDFD128BB0F76C8787444E15D31F170677112B0A54ACC18EABB5050E867B7D"
                                     "AE4EEC13CE626112AD628A63018B57F0450E372997F86C88E1CC75ECC2A4CF12EA45DA6CC3"
                                     "65253EA0A82476DC8D8E039777A9DD9865C289BBAF78F4A002BBD1D967F08F8FEF68EFEBD0"
                                     "8F5982273759C116EE7D75A879DBD56A5C2B55110E3002A69E14E6501A86F5"};

const uint8_t sha_512_expected_00[] = {"6D0C1F3DC4F6EC9C1EB9D3C8F191743EF98875B6F121DE1FF803ED129BFE39A69998B1BC23"
                                       "EC686AAAE83E463B5C6CC3B5542054275C11BC0AEDF92043FA279C"};
const uint8_t sha_512_expected_01[] = {"2846C156124199C4FA1A9E83BF98DDD96495F29F431A53CD552F918AB0863B92954B20FDAD"
                                       "198ED57D95DECAB0DAEB23C048565D061B7EC71AF133497F3B0C35"};
const uint8_t sha_512_expected_02[] = {"EF64333748F13EBF477804423207FFBE8E2F185C9ECE9E662DE0AC8DE87AB4E39C9AAD8164"
                                       "0B926042B628FE08A0A92D6CAA631749C195310DE21EBCF363CC24"};
const uint8_t sha_512_expected_03[] = {"051B46DAF7D78AB2812DCDEF977CB1F3F79FEA5D53A2F0AEE2DF7685C8D624B77C3873DC6E"
                                       "AE311670607CFBEC46309FD8A9DC830505024E4CBC96CE1D2646A4"};
const uint8_t sha_512_expected_04[] = {"331AF9FD46D5D88D25C409570A7D36C146908B7EB6FD8FCD2DCDF43327AAADC407E9F432B1"
                                       "A8255F3B96CF7DFF3726108BC1D0E28BA8710766981C54EAC37F44"};
const uint8_t sha_512_expected_05[] = {"1AE11B9F69EDE0A792BB7A8372276A66B2C61AC00B1FB8239033EF187EE8E60F6CC5873CAA"
                                       "82F7C713E7A3BD5F4AF43DA386AEF22C45B78117B0F22045B632DD"};

const HASH_InitTypeDef sha_512_configer = {SEA_SHA_512, HASH_GENERAL_MODE_EN, {0, 0}};

const SEA_HASH_TESTLIST sha_512_test_00 = {(uint32_t *)sha_512_requst_00, sizeof(sha_512_requst_00) >> 1, (uint32_t *)sha_512_expected_00};
const SEA_HASH_TESTLIST sha_512_test_01 = {(uint32_t *)sha_512_requst_01, sizeof(sha_512_requst_01) >> 1, (uint32_t *)sha_512_expected_01};
const SEA_HASH_TESTLIST sha_512_test_02 = {(uint32_t *)sha_512_requst_02, sizeof(sha_512_requst_02) >> 1, (uint32_t *)sha_512_expected_02};
const SEA_HASH_TESTLIST sha_512_test_03 = {(uint32_t *)sha_512_requst_03, sizeof(sha_512_requst_03) >> 1, (uint32_t *)sha_512_expected_03};
const SEA_HASH_TESTLIST sha_512_test_04 = {(uint32_t *)sha_512_requst_04, sizeof(sha_512_requst_04) >> 1, (uint32_t *)sha_512_expected_04};
const SEA_HASH_TESTLIST sha_512_test_05 = {(uint32_t *)sha_512_requst_05, sizeof(sha_512_requst_05) >> 1, (uint32_t *)sha_512_expected_05};

const SEA_HASH_TESTLIST *sha_512_testlist[] = {(SEA_HASH_TESTLIST *)&sha_512_test_00, (SEA_HASH_TESTLIST *)&sha_512_test_01, (SEA_HASH_TESTLIST *)&sha_512_test_02,
                                               (SEA_HASH_TESTLIST *)&sha_512_test_03, (SEA_HASH_TESTLIST *)&sha_512_test_04, (SEA_HASH_TESTLIST *)&sha_512_test_05};

static uint32_t hash_testunit(const hashData_t *pMsg, const HASH_InitTypeDef *hashcfg, hashData_t *p_dataout, uint32_t max_size, uint32_t ret_data_size) {
    uint32_t i;
    uint32_t loops, calcDataSize;
    CYPHER_DATA_STA is_last;
    HASH_HandleTypeDef pParas;
    uint32_t tmp_err = SEA_SUCCESS;

#ifdef HASH_PROCESS_LATCH_RECOVER_SUPPORT
    HASH_HandleTypeDef tmp_latch_buf;
    aesProcessParameters ptmp;
#endif

    tmp_err = LL_HASH_init(&pParas, (const HASH_InitTypeDef *)hashcfg);
    TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);

    loops = ((ret_data_size % max_size) != 0U) ? ((ret_data_size / max_size) + 1) : (ret_data_size / max_size);
    for (i = 0; i < loops; i++) {
        WDT_FEED;
#ifdef HASH_PROCESS_LATCH_RECOVER_SUPPORT
        if ((i == 0) || (i == 3)) {
#ifdef HASH_SHA384_TEST_SUPPORT
            printf("[log]++hash_latch recover begin\n");
            LL_HASH_process_status_latch(&pParas, &tmp_latch_buf);
            hash_384_demo_single_calc();
            LL_HASH_process_status_recover(&tmp_latch_buf, &pParas);
            printf("[log]++hash_latch recover finish\n");
#endif
        } else if (i == 1) {
            printf("[log]++hash_latch recover begin\n");
            LL_HASH_process_status_latch(&pParas, &tmp_latch_buf);
//            aes_128_gcm_demo_init(&ptmp, AES_deCrypt);
//            aes_128_gcm_demo_decrypt_verify(&ptmp);
            LL_HASH_process_status_recover(&tmp_latch_buf, &pParas);
            printf("[log]++hash_latch recover finish\n");
        }
#endif
        calcDataSize = (ret_data_size > max_size) ? max_size : ret_data_size;
        ret_data_size -= calcDataSize;
        is_last = (ret_data_size) ? CypherDatasNotTheLast : CypherDatasIsTheLast;
        tmp_err = LL_HASH(&pParas, (const hashData_t *)((uint32_t)pMsg + (i * max_size)), calcDataSize, is_last, (hashData_t *)p_dataout);
        TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);
    }
    (void)LL_HASH_deinit();

    return tmp_err;
}

void hash_test(void) {
    uint32_t i, cnt;
    uint32_t tmp_err = SEA_SUCCESS;
#ifdef HASH_CRYPT_ITEMS_IN_BYTES
    hashData_t tmp_hash_data_buf[2048 + 1];
    hashData_t tmp_dataout_buf[NOUT_SHA_MAX * 4 + 1];
    hashData_t *tmp_hash_data = (hashData_t *)&tmp_hash_data_buf[1];
    hashData_t *tmp_dataout = (hashData_t *)&tmp_dataout_buf[1];
#else
    hashData_t tmp_hash_data_buf[2048 / 4];
    hashData_t tmp_dataout_buf[NOUT_SHA_MAX];
    hashData_t *tmp_hash_data = (hashData_t *)&tmp_hash_data_buf[0];
    hashData_t *tmp_dataout = (hashData_t *)&tmp_dataout_buf[0];
#endif

    uint32_t tmp_check_data[2048 / 4], *verify_message;
    const HASH_InitTypeDef *hash_cfg;
    const hashData_t *message_in;
    hashData_t *message_out;
    uint32_t calcDataSize, outDataSize, testBufSize;
    AES_KernelTypeDef version = ((sea_update_stat()) ? AES_RUNNING_IN_CORE0 : AES_RUNNING_IN_CORE1);

#ifdef HASH_SHA1_TEST_SUPPORT
    printf("</***********HASH SHA1 test start***********\n");
    for (cnt = 0; cnt < 2; cnt++) {
        testBufSize = (cnt + 1) * (1 << BSHIFT_BLOCK_DATA_SHA_160_256);
        sea_init_all(version);

        for (i = 0; i < (sizeof(sha_160_testlist) / sizeof(sha_160_testlist[0])); i++) {
            WDT_FEED;
            hash_cfg = (const HASH_InitTypeDef *)&sha_160_configer;
            message_in = tmp_hash_data;
            message_out = tmp_dataout;
            verify_message = tmp_check_data;
            // get input size and output size
            calcDataSize = sha_160_testlist[i]->msglen;
            outDataSize = hashOutSize((HASH_InitTypeDef *)hash_cfg);

            // str2hex: input message and verify message
            str2hex((const uint8_t *)sha_160_testlist[i]->msg, (uint8_t *)message_in, calcDataSize);
            str2hex((const uint8_t *)sha_160_testlist[i]->expected, (uint8_t *)verify_message, outDataSize);

            tmp_err = hash_testunit((const hashData_t *)message_in, hash_cfg, message_out, testBufSize, calcDataSize);
            TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);

            if (0 != memcmp((const uint8_t *)message_out, (const uint8_t *)verify_message, outDataSize)) {
                printf("++SHA1 case%d test error, buffsize: %d, bitwide: %d\n", i, testBufSize, calcDataSize * 8);
                printf("***********HASH SHA1 test Fail!!!***********/>\n");
                return;
            } else {
                printf("++SHA1 case%d test success, buffsize: %d, bitwide: %d\n", i, testBufSize, calcDataSize * 8);
            }
        }
    }

    printf("***********HASH SHA1 test Pass***********/>\n");
#endif

#ifdef HASH_SHA224_TEST_SUPPORT
    printf("</***********HASH SHA224 test start***********\n");
    for (cnt = 0; cnt < 2; cnt++) {
        testBufSize = (cnt + 1) * (1 << BSHIFT_BLOCK_DATA_SHA_160_256);
        sea_init_all(version);

        for (i = 0; i < (sizeof(sha_224_testlist) / sizeof(sha_224_testlist[0])); i++) {
            WDT_FEED;
            hash_cfg = (const HASH_InitTypeDef *)&sha_224_configer;
            message_in = tmp_hash_data;
            message_out = tmp_dataout;
            verify_message = tmp_check_data;
            // get input size and output size
            calcDataSize = sha_224_testlist[i]->msglen;
            outDataSize = hashOutSize((HASH_InitTypeDef *)hash_cfg);

            // str2hex: input message and verify message
            str2hex((const uint8_t *)sha_224_testlist[i]->msg, (uint8_t *)message_in, calcDataSize);
            str2hex((const uint8_t *)sha_224_testlist[i]->expected, (uint8_t *)verify_message, outDataSize);

            tmp_err = hash_testunit((const hashData_t *)message_in, hash_cfg, message_out, testBufSize, calcDataSize);
            TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);

            if (0 != memcmp((const uint8_t *)message_out, (const uint8_t *)verify_message, outDataSize)) {
                printf("++SHA224 case%d test error, buffsize: %d, bitwide:%d\n", i, testBufSize, calcDataSize * 8);
                printf("***********HASH SHA224 test Fail!!!***********/>\n");
                return;
            } else {
                printf("++SHA224 case%d test success, buffsize: %d, bitwide:%d\n", i, testBufSize, calcDataSize * 8);
            }
        }
    }

    printf("***********HASH SHA224 test Pass***********/>\n");
#endif

#ifdef HASH_SHA256_TEST_SUPPORT

    printf("</***********HASH SHA256 test start***********\n");
    for (cnt = 0; cnt < 2; cnt++) {
        testBufSize = (cnt + 1) * (1 << BSHIFT_BLOCK_DATA_SHA_160_256);
        sea_init_all(version);

        for (i = 0; i < (sizeof(sha_256_testlist) / sizeof(sha_256_testlist[0])); i++) {
            WDT_FEED;
            hash_cfg = (const HASH_InitTypeDef *)&sha_256_configer;
            message_in = tmp_hash_data;
            message_out = tmp_dataout;
            verify_message = tmp_check_data;
            // get input size and output size
            calcDataSize = sha_256_testlist[i]->msglen;
            outDataSize = hashOutSize((HASH_InitTypeDef *)hash_cfg);

            // str2hex: input message and verify message
            str2hex((const uint8_t *)sha_256_testlist[i]->msg, (uint8_t *)message_in, calcDataSize);
            str2hex((const uint8_t *)sha_256_testlist[i]->expected, (uint8_t *)verify_message, outDataSize);

            tmp_err = hash_testunit((const hashData_t *)message_in, hash_cfg, message_out, testBufSize, calcDataSize);
            TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);

            if (0 != memcmp((const uint8_t *)message_out, (const uint8_t *)verify_message, outDataSize)) {
                printf("++SHA256 case%d test error, buffsize: %d, bitwide:%d\n", i, testBufSize, calcDataSize * 8);
                printf("***********HASH SHA256 test Fail!!!***********/>\n");
                return;
            } else {
                printf("++SHA256 case%d test success, buffsize: %d, bitwide:%d\n", i, testBufSize, calcDataSize * 8);
            }
        }
    }

    printf("***********HASH SHA256 test Pass***********/>\n");
#endif

#ifdef HASH_SHA384_TEST_SUPPORT
    printf("</***********HASH SHA384 test start***********\n");
    for (cnt = 0; cnt < 2; cnt++) {
        testBufSize = (cnt + 1) * (1 << BSHIFT_BLOCK_DATA_SHA_384_512);
        sea_init_all(version);

        for (i = 0; i < sizeof(sha_384_testlist) / sizeof(sha_384_testlist[0]); i++) {
            WDT_FEED;
            hash_cfg = (const HASH_InitTypeDef *)&sha_384_configer;
            message_in = tmp_hash_data;
            message_out = tmp_dataout;
            verify_message = tmp_check_data;
            // get input size and output size
            calcDataSize = sha_384_testlist[i]->msglen;
            outDataSize = hashOutSize((HASH_InitTypeDef *)hash_cfg);

            // str2hex: input message and verify message
            str2hex((const uint8_t *)sha_384_testlist[i]->msg, (uint8_t *)message_in, calcDataSize);
            str2hex((const uint8_t *)sha_384_testlist[i]->expected, (uint8_t *)verify_message, outDataSize);

            tmp_err = hash_testunit((const hashData_t *)message_in, hash_cfg, message_out, testBufSize, calcDataSize);
            TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);

            if (0 != memcmp((const uint8_t *)message_out, (const uint8_t *)verify_message, outDataSize)) {
                printf("++SHA384 case%d test error, buffsize: %d, bitwide:%d\n", i, testBufSize, calcDataSize * 8);
                printf("***********HASH SHA384 test Fail!!!***********/>\n");
                return;
            } else {
                printf("++SHA384 case%d test success, buffsize: %d, bitwide:%d\n", i, testBufSize, calcDataSize * 8);
            }
        }
    }

    printf("***********HASH SHA384 test Pass***********/>\n");
#endif

#ifdef HASH_SHA512_TEST_SUPPORT
    printf("</***********HASH SHA512 test start*********\n");
    for (cnt = 0; cnt < 2; cnt++) {
        testBufSize = (cnt + 1) * (1 << BSHIFT_BLOCK_DATA_SHA_384_512);
        sea_init_all(version);

        for (i = 0; i < (sizeof(sha_512_testlist) / sizeof(sha_512_testlist[0])); i++) {
            WDT_FEED;
            hash_cfg = (const HASH_InitTypeDef *)&sha_512_configer;
            message_in = tmp_hash_data;
            message_out = tmp_dataout;
            verify_message = tmp_check_data;
            // get input size and output size
            calcDataSize = sha_512_testlist[i]->msglen;
            outDataSize = hashOutSize((HASH_InitTypeDef *)hash_cfg);

            // str2hex: input message and verify message
            str2hex((const uint8_t *)sha_512_testlist[i]->msg, (uint8_t *)message_in, calcDataSize);
            str2hex((const uint8_t *)sha_512_testlist[i]->expected, (uint8_t *)verify_message, outDataSize);

            tmp_err = hash_testunit((const hashData_t *)message_in, hash_cfg, message_out, testBufSize, calcDataSize);
            TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);

            if (0 != memcmp((const uint8_t *)message_out, (const uint8_t *)verify_message, outDataSize)) {
                printf("++SHA512 case%d test error, buffsize: %d, bitwide:%d\n", i, testBufSize, calcDataSize * 8);
                printf("***********HASH SHA512 test Fail!!!***********/>\n");
                return;
            } else {
                printf("++SHA512 case%d test success, buffsize: %d, bitwide:%d\n", i, testBufSize, calcDataSize * 8);
            }
        }
    }

    printf("***********HASH SHA512 test Pass***********/>\n");
#endif
}
