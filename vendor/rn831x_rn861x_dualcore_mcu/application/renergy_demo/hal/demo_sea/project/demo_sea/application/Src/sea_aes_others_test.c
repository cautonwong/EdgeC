#include "sea_test.h"

#define AES_OTHERS_CRYPT_TEST_ENABLE
#define AES_OTHERS_DECRYPT_TEST_ENABLE

static uint32_t aes_other_testunit(SEA_AES_OTHERS_TESTLIST *testList, uint32_t buff_size, AES_ENCRYPT_SEL encrypt_sel);

#define AES_OTHERS_CRYPT_TEST_ENABLE
#define AES_OTHERS_DECRYPT_TEST_ENABLE

const uint32_t others_buffsize_testlist[] = {16U, 64U, 128U, 512U, SEA_STACK_MAX_SIZE};

const uint8_t aes_bitwide_str[3][8] = {"AES_128\0", "AES_192\0", "AES_256\0"};
const uint8_t aes_mod_str[6][8] = {"AES_ECB\0", "AES_CBC\0", "AES_CTR\0", "AES_CFB\0", "AES_OFB\0", "AES_GCM\0"};

/* AES ECB */
const uint8_t aes_ecb_pt_0[] = {"F34481EC3CC627BACD5DC3FB08F273E6"};
const uint8_t aes_ecb_ky_0[] = {"00000000000000000000000000000000"};
const uint8_t aes_ecb_ct_0[] = {"0336763E966D92595A567CC9CE537F5E"};

const uint8_t aes_ecb_pt_1[] = {"9CB07E739C68AB7B13CB73982528AF25BF6EEDB9D94CA2BD3C183889107C754DC324F0590D"
                                "871EC0E6E7EF9F2B8B5D82"};
const uint8_t aes_ecb_ky_1[] = {"3EC87BD0DD9568A9DA2BFFC8A9E8C8AFD32C0EA3FD6E4E454940202BCB0373E5"};
const uint8_t aes_ecb_ct_1[] = {"A2FE4674251244F5435DABD08F2D540E3CA71D3AF29CD8F0EEFBE8F26834F0DF56107B59A3"
                                "F1B11D5C5A9A46C235A70A"};

const uint8_t aes_ecb_pt_2[] = {"00000000000000000000000000000000"};
const uint8_t aes_ecb_ky_2[] = {"E9F065D7C13573587F7875357DFBB16C53489F6A4BD0F7CD"};
const uint8_t aes_ecb_ct_2[] = {"0956259C9CD5CFD0181CCA53380CDE06"};

const uint8_t aes_ecb_pt_3[] = {"B84BB6B88EE916EDBF1FDC4E7D90F9FF07AC11E0664B11407FCDF5B0865C54C4FB21BC7CD4"
                                "F1429EE7FB87FA5F8C15C2F252132593493874C78BAAD088FDE8471A3CDA9353EB521BD1A4"
                                "2A9FBFD9CB299D7DAE6EA36F01D5B40F67270C1442119A47D32BB5B33FA0C1B794941FED83"
                                "2FB2B6E4CA0E838EEE679046938C14D266"};
const uint8_t aes_ecb_ky_3[] = {"89C63E0B9A989C34E7ACA42B82D4EA958C06953CE91F0C48CC41CCB7D0CE4E17"};
const uint8_t aes_ecb_ct_3[] = {"C588595CAAABCC1CA92C91BDC3D6A829D214E1C6B27D9CF72A1F8E73AD0E918C822E79F88A"
                                "9F9E31397B5DA6BE3642141FC6C21057EB365AD0C3C6F13EEFEEA49E17C8C1D99E310A0E07"
                                "94C9C5627827FFA213175031DAA6A129B7193103B8E3C9EF16F82C4136DCA8196C76EBCEAC"
                                "6B4BD40144C7EFB6908D612487DEC46E66"};

/* AES CBC */
const uint8_t aes_cbc_pt_0[] = {"F34481EC3CC627BACD5DC3FB08F273E6"};
const uint8_t aes_cbc_iv_0[] = {"00000000000000000000000000000000"};
const uint8_t aes_cbc_ky_0[] = {"00000000000000000000000000000000"};
const uint8_t aes_cbc_ct_0[] = {"0336763E966D92595A567CC9CE537F5E"};

const uint8_t aes_cbc_pt_1[] = {"9EE2B2D6D3F35187EB207B68138A1722DA4EAC8CDD6159DA198951436B1B8C5D7499BB537B"
                                "ABFD5A94962DC0AAA79872358DE3124BE78BE3226F942443B1ED5363E2D3F8ABB2C7AF82A7"
                                "1036C198276063F5EE39722A4A499ECF16CB3983246EEEB9BB11EB4F06C3BA7F69824133E5"
                                "DA42B76269C068B6BA9E73073A84ECDA6765BBA01B59FA619416D3FF81D549AB9A"};
const uint8_t aes_cbc_iv_1[] = {"D823D3884E6E4ED7D716C3B78F4D32AE"};
const uint8_t aes_cbc_ky_1[] = {"F15B85935632EF01169F559446CFCB9A80E5945D7BB9F415C35E68B37C8DE785"};
const uint8_t aes_cbc_ct_1[] = {"F53156B7D617916D804F41428311353FCA404476D327DF47125CAAE7A0705058E2D8A67B0F"
                                "95DAC55C67855B0107716D460823CDE6C95D9BAD654DB1DDEDD505AA4349F47F7C48EE0E2B"
                                "4031C38DAD600E87D57575FB9B2BCD25F75783E8F09D9848E9AE677FF51B7E99C982F52BC0"
                                "A5BCD19ACD8A6D4E94F1B0747597C687D7AD355C84326931A85560E3C48F8DDC42"};

const uint8_t aes_cbc_pt_2[] = {"56F5412E4098F0E8A976AB8CF21BE443065EB6DE93A66A02E680989E567ED0E518ABD087DE"
                                "3E69F22A9C29252C02E4717C7E8BD0843785B965BA5A4F22FDAC59226FBFDA5BD3DFDF75CD"
                                "69A639A327C5AF1F0E3F23A438BF1043E946BBB8F730BC5CFAEB0B1A063586542F36C7467A"
                                "9C1E7E7722DA22BC9DDF34B35447393A75"};
const uint8_t aes_cbc_iv_2[] = {"94CE4CF23A120F877B579A734A39F747"};
const uint8_t aes_cbc_ky_2[] = {"D9AF5D0E25C0B60398B58A78F6AA6C87BA3ED7CC1650BD9A"};
const uint8_t aes_cbc_ct_2[] = {"906FA9C5C32227037EAFCAD90F8A9E6C9B6342F7ADA0F7241D5E96723C75F7A3886C99C32D"
                                "5166DBAD3718EDCF89E550B014420EF446072933D61729FED53B979D9963F5DB4F80DE7136"
                                "3EB5CA2BFB34889447770C943DEC627413AD2FD712EBECE318CABE6EE5F88B4FA231326A9E"
                                "4784460110C1277AB5642163C1480C2D13"};

const uint8_t aes_cbc_pt_3[] = {"D6D4252A87FE42526E1AF6E3775C47A82786BBA32D240671E36B0EAD7D26AA5F0257020B55"
                                "1E7F0A4D1829C406304A2C122E617695D54E49519007CDA203C5A95659E34F4A5B10FD848D"
                                "29D7A8B15626C1558BA8146DF4E8358A7CA7E846D49CBA54BC2E2CB8A48DB12042CC87D495"
                                "2A156D280AAA88BC44DBB67B8559F4EC0F"};
const uint8_t aes_cbc_iv_3[] = {"E21DFBD0B12B780FB1F063BC73BC56B4"};
const uint8_t aes_cbc_ky_3[] = {"EDCFAB3B4E2E225AE2828E9B0BFCF298F5E5545464660EC2C60E3B66E7527ACE"};
const uint8_t aes_cbc_ct_3[] = {"6F5232F264C1CE37715F36C0EE3F1EED564A0EEE8AC6071FC210FD1C6593BA19437DDA29E9"
                                "49E303D4DF1A6B85D1D3C42BA1FAA16BBD4C4A7B1B3B47C5A2178CCABC91C104498E51EB79"
                                "ACD710823AFE58C3BF7D2084D5D53FDE72789ACE7D80B62BEDC0CF2B87EB1F1A2640928B29"
                                "EE93D6164223CEB7B87C61ED8B34AD6C12"};

/* AES CTR */
const uint8_t aes_ctr_pt_0[] = {"A6A33702480DEF953D4A706A6E65B858"};
const uint8_t aes_ctr_iv_0[] = {"C2E756F7CCF0BE67B488147C74C4A9E8"};
const uint8_t aes_ctr_ky_0[] = {"7296BCF4CCC767B70534D58FC35E2ED9"};
const uint8_t aes_ctr_ct_0[] = {"F2789C18D7A38B21FED0514D54B5A02B"};

const uint8_t aes_ctr_pt_1[] = {"5F792CEC9D94DF1E1D0B7FCA3BA71C28E6A2BF71CA56BB68D253CAB49DA24E90E84851B1C1"
                                "CC3D9722F75602AC14AA73D6621BACA14482B02BEDB8DB2B0115730D2BDEE61AF28B014F60"
                                "B54D07E6568A2EF6B6B3165CBBC72E6CA44CCD8CF7D7445EE8039FD9698C1631085D0BB11D"
                                "CF39856823D95AD2F62E7DBF847FA13257BDC0FE25B5901097198F0DE852EA45EEF023ED00"
                                "E4B59842839E93AF70821C231A5B47509F0DF327854CD8729D5695C22D30069CC44DB20411"
                                "9E33A9B796E3F101AA3EE8AC721FF7BFB24D97BAAAE1B6474B4554741ECD5943F4C2BE05B2"
                                "D9D2E3921C7E0CDC0D73847BBA92A6F6F9D45C385C9069C7841E284BC5B9E5A4737A44F38D"
                                "B37E3EEFA3DCAE1BF83CF6A408ADA862782D1EC671459ABCFC80E922A073F6894AFEA67DBF"
                                "B189AF2F39C9B9E54745A20844E770CE3E8C5AFBFDA02C8BDE55C2F5DCEC4E50B1A75F9DEC"
                                "04FE9F96BA9CAD7400F536EC65374DF72E6711269B3337694F9FE4F317083A1D"};
const uint8_t aes_ctr_iv_1[] = {"FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFE3"};
const uint8_t aes_ctr_ky_1[] = {"111D0CA99498E69AD8B02CFF46434157A821C5785D105C3A"};
const uint8_t aes_ctr_ct_1[] = {"8841FE941855C5FEA6BA671B8E57DC60E0FA8FFC0D2A666F680D7DA02CBC30D52F2F11995F"
                                "80F1E0F08924BAD8F9942A1AA3B3BD92C95FA586FE6ADF941453CFD2C63500343A478F0DDB"
                                "C9EAE31CF7DCB903F8F011C85ECDDF6001996632637995FBC78C34D9A85055F316DA1E41E4"
                                "F575DD455E9EE3D8B9223D1178D4EFD61828ECB21B5CDAD175278B3F672488DAD3703E2E1A"
                                "612343EA80F995D361A2B8B5DCE07A580C6678BB9567C7A86D13DA72B19DF36FDEABCA83D4"
                                "C13AA94645C0DD1896E3BF98058E2B8607A24918A2AFA6AE0EE7DAFB7D55408C855AF75549"
                                "E4C4F53D0DD1337CEAC8293557282A543CDC178C9A785BC9E404590FB815B80CD5E8EC13BD"
                                "FB56E394A0BF6EC80E21360AA4DE1DC1AB15AED7E72120984FA2AA9C0F78E0B0629651B97D"
                                "4FB7ED9D4E5C0910CE6CB40F757450826F9E896F693B0DF8709FCDAEA2D9EF5F7472CAA2A2"
                                "6D2D2639F0C94C0FAAB4D9CE076BDDF329B33186361095B96CEF6878FB0279CC"};

const uint8_t aes_ctr_pt_2[] = {"00000000000000000000000000000000"};
const uint8_t aes_ctr_iv_2[] = {"00000000000000000000000000000000"};
const uint8_t aes_ctr_ky_2[] = {"FFFFFF8000000000000000000000000000000000000000000000000000000000"};
const uint8_t aes_ctr_ct_2[] = {"BD49295006250FFCA5100B6007A0EADE"};

const uint8_t aes_ctr_pt_3[] = {"44DF1EDBCC39CD81596F490BEE6924B23874FAAB76C61F6E9FE9B9A32F332FA273BC6C47B8"
                                "A4B85245C0DE1B3FE8928CDA489C75574ED739FA5FCA40811E7551F5DAD4D14A9BA7A5C147"
                                "D8A780711220E808644AD4F506527AEBDB40ED5C976747984722480DEAD47921133D4892A0"
                                "6E6FF9495337D6713B00E45667BEA445CF"};
const uint8_t aes_ctr_iv_3[] = {"FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFB"};
const uint8_t aes_ctr_ky_3[] = {"779DF34B6CF8D9071DC6D769594F75A2B1149C900721A0928E6BCB832AF861EA"};
const uint8_t aes_ctr_ct_3[] = {"DFC6C62504BCD0578C97BFFDB320EC684E7F061E36B9473DC2F7073D4F659EDCC38A8A46B7"
                                "C321B832D34A4E5F628A96A81AE9C8D42574690C7EE82574032AD7E51E5F3DB695833747DB"
                                "43AB2353BBAB1D7B417A3C84FA23C740DB3BFB6993767E6DA21A0DA711EA95264DCF483CAA"
                                "E7B9E48F077AA94488396DE5C87F195D94"};

const uint8_t aes_ctr_pt_4[] = {"EBAD42B816845199FC98EE7318ED2968D5BE52DA8324A239236BAB6A75E00E7DA990EEFA66"
                                "8ED21BDFBBC5AEB2D844EB4E85030D1E9E952FCC84B527DD70535D3DC5314E23E8CC2EA91B"
                                "AC9CD8E2D8DBC2083D0D7D6FD704FC590434887618A79643A2CE8193BC6BE9C6BF3925C979"
                                "7F4F956444A48F0658737258D1DC0631F0"};
const uint8_t aes_ctr_iv_4[] = {"0000000000000000000000000000000F"};
const uint8_t aes_ctr_ky_4[] = {"42BD508F541AD8F50C8DEFA8A04F2BC72BB3CE40262BF4D461EB2DB28B28EABA"};
const uint8_t aes_ctr_ct_4[] = {"629E63981FAFDDDF43C1E6105230613B76895D5C1AD7F0A555703E32FA1BB6F39E41D9028A"
                                "A250880FE91717D14A459F1E27CE421B89FC3BA0EE3A6F37EDC85AD9B75A97D0806345CB78"
                                "E51713919490E0761FC2758FAAF9D9A982EEE71C01F4002AA965DA25163EEA44D2C682D6E5"
                                "22DC228C39E1C885821C35BDE6BE4D899D"};

/* AES CFB128 */
const uint8_t aes_cfb128_pt_0[] = {"00000000000000000000000000000000"};
const uint8_t aes_cfb128_iv_0[] = {"F34481EC3CC627BACD5DC3FB08F273E6"};
const uint8_t aes_cfb128_ky_0[] = {"00000000000000000000000000000000"};
const uint8_t aes_cfb128_ct_0[] = {"0336763E966D92595A567CC9CE537F5E"};

const uint8_t aes_cfb128_pt_1[] = {"DDCE35356D95BDBB4EDC47D7F09FAE4DF62204F42CEC6927FD2B90C47B38D85EF55853D529"
                                   "ED78C76A87690A50F2FCC6C67D240B94208630A35E23E55D2843B8E797CAEC6575CA1B2E99"
                                   "14B31D0C852011EEECBEF1BFF6BF45B84AF308A6607F"};
const uint8_t aes_cfb128_iv_1[] = {"4B0BEFCEC9FE0F9BF2C8BEFA06BFEAFB"};
const uint8_t aes_cfb128_ky_1[] = {"171D68B326DBF655C2755A7CA129F5D1350201EB33352A2D"};
const uint8_t aes_cfb128_ct_1[] = {"5568AD855B265A295B2265A92D500FFCA1CF35786D31E499D6FBC6A121BF6720DA0724E412"
                                   "C2684854153ED65AE5EB18500ADB466500D1C7DA55484679FB892949D03D19D6C68F69DEF7"
                                   "844605F4D5404B389B7D62D8661E76D6A78DB4D1B3DE"};

const uint8_t aes_cfb128_pt_2[] = {"D272F9399F8573B98D12AD61AC6046774588EF7E0CBB604B652B431A36CC101B3FA7796E7D"
                                   "768013738BC25A23B2C9EFAE72FCCE7FDEE74CE177AF1F98E6444F6DD014754FF5EA41566F"
                                   "5CF30841C2154BDB017B1B7DEC531CC96367B7D682D07FCA9CE6C814CAB0EE184C6C320415"
                                   "45"};
const uint8_t aes_cfb128_iv_2[] = {"3A8617D6130EB4B33B15CA8ADFCBE9ED"};
const uint8_t aes_cfb128_ky_2[] = {"522C8CBEA77D3FFE6DDEFB19280530F824F47F06FAD74D9DAC933B30BD649E40"};
const uint8_t aes_cfb128_ct_2[] = {"C7DD0914FE4D44E6394A5C57AE1C42E8FB17AAB2715E15895A815B0A5A2C4D5CC03D023D11"
                                   "4A8319E7226257FC1BC2575B6139FEC8FA4E88362478B47B212CB21F154ED3FCC4CBBDFE58"
                                   "1DF80D91BC3BA3EAA5BE3D8240D34ED92C7BEDBEF5B14E048BDB0A157BB0021EFA5575019E"
                                   "1A"};

const uint8_t aes_cfb128_pt_3[] = {"CCB1E3457F6BF1AC3E200F9E4DFDD4169AA747957E5FE677D600B113DE4792F33E14114E7D"
                                   "709A1A104BFFD936EAC3CA8B5DF16B26F15EDE0061B61A8501F83DF9468911C85F748B136E"
                                   "86DC3DE6D88C1A3615D6858B39C725DAAEF1FCAD8897283DBF0F77DD00A13477F660B983B8"
                                   "AE52B3C70E98207BB95500EFC901A8030A"};
const uint8_t aes_cfb128_iv_3[] = {"DF9D2D51898AE45B97F0CE14E1349234"};
const uint8_t aes_cfb128_ky_3[] = {"9490804522320971E3A0C6137A67D6F2D27BCDA0F2BE14C319787EAAD933B349"};
const uint8_t aes_cfb128_ct_3[] = {"C80EDB8FC000581426244DB528ADEB1C51AD1AC99B7A183A2B82DDEC0A4215565750B9885B"
                                   "65D79FB9AC4059AED6DE56A20906B9C3386964DD08DBDC155EB7EDAAF99AE59E82CED14461"
                                   "8003BEA6CE8743389BD605FBD895D98B97339AB3CB8E7AF3811371978F2CF46D25E6BBC812"
                                   "79FD4A451D09B1F6DA9512BCF5969E1E7D"};

/* AES OFB */
const uint8_t aes_ofb_pt_0[] = {"00000000000000000000000000000000"};
const uint8_t aes_ofb_iv_0[] = {"F34481EC3CC627BACD5DC3FB08F273E6"};
const uint8_t aes_ofb_ky_0[] = {"00000000000000000000000000000000"};
const uint8_t aes_ofb_ct_0[] = {"0336763E966D92595A567CC9CE537F5E"};

const uint8_t aes_ofb_pt_1[] = {"6C1D261EC1250CAD09C3CB57E5C87536A9312EC00A3CA851DCAC31482BF11F6024299D5049"
                                "FB1E5124D523741A4A57BAEB0960E489AEAF096D701ADF7631832C37E5A13C83613F0915B7"
                                "6FBA4616C1F6"};
const uint8_t aes_ofb_iv_1[] = {"583A6C053685D08B2ABDEB0C3B7DE85C"};
const uint8_t aes_ofb_ky_1[] = {"95CD44D5DA1352EC2263CAE6CCC96EAB88A3DE6EC5288A85"};
const uint8_t aes_ofb_ct_1[] = {"0F8CD69EDBE61BFA5DD8E0B59FC1A91A219AC27684EB79B49FC04F087EEC4DC9F3174B42B7"
                                "06AD41A370DB94D8B0F510768395C42CCB385C43C3926CB511F0C2DEB55CA8A9A1B97CBEF2"
                                "5CB1E90C1491"};

const uint8_t aes_ofb_pt_2[] = {"5D2FDEDAED153E7F35F89B4831D116CA42F63E0A22C1995D20233B44F62E74DD"};
const uint8_t aes_ofb_iv_2[] = {"570DA9E953B5A9FC271F4C51FAF036E9"};
const uint8_t aes_ofb_ky_2[] = {"56A64D497ACA757F1BFDE041AF673E8BACBF415E9698ED80"};
const uint8_t aes_ofb_ct_2[] = {"322B50B4AC435D6795ED383FBFEC13996C40824135DB2777D3F8955757CCD78F"};

const uint8_t aes_ofb_pt_3[] = {"958E002A9FDA9D8937CA343772E59B3DE95303E2C44E3153DA6AAAB2CC91AF164948DD371F"
                                "F00485984D52764A63192E098797E2DEA2BD17EF80BB43528D8931163B88FA73C5C7B3F6E0"
                                "C9035D705F756F008FD4A5B84494E165BD1461FA2F9E1F5AEBC202FE6777A2EABAAEC2F7E8"
                                "EB16074BDAB3C40DA67DFEB75A4B7611C0"};
const uint8_t aes_ofb_iv_3[] = {"E4858C38DC5DD7AFD90C27E93B08E59A"};
const uint8_t aes_ofb_ky_3[] = {"07C316FFA772F1C08E00B2EACB6F8DBC8264543EED2F335C780D630D6CB9C0BE"};
const uint8_t aes_ofb_ct_3[] = {"610128540AA9B539C37A6710BA50BC3C1994A2FBC6469C2F1720A85DACB53BF5106D3A87AC"
                                "A75124601B858A5E4B7D216A0FC3A7AFD12B9B89D4FB5CCE16544904C65B3871274CDD3C52"
                                "CCF1E22603137D777301FF3FBC6CA5ACAEF8858E772D0D67B0615FFA98DD10C3E76B3DD047"
                                "38CC80EA454361B0B428BA320A045CA024"};

/* AES ECB */
const SEA_AES_OTHERS_TESTLIST aes_ecb_0 = {(uint32_t *)aes_ecb_ky_0,
                                           (uint32_t)sizeof(aes_ecb_ky_0) >> 1,
                                           (uint32_t *)aes_ecb_pt_0,
                                           (uint32_t)sizeof(aes_ecb_pt_0) >> 1,
                                           (uint32_t *)0,
                                           0,
                                           (uint32_t *)aes_ecb_ct_0,
                                           AES_128,
                                           AES_ECB};

const SEA_AES_OTHERS_TESTLIST aes_ecb_1 = {(uint32_t *)aes_ecb_ky_1,
                                           (uint32_t)sizeof(aes_ecb_ky_1) >> 1,
                                           (uint32_t *)aes_ecb_pt_1,
                                           (uint32_t)sizeof(aes_ecb_pt_1) >> 1,
                                           (uint32_t *)0,
                                           0,
                                           (uint32_t *)aes_ecb_ct_1,
                                           AES_256,
                                           AES_ECB};

const SEA_AES_OTHERS_TESTLIST aes_ecb_2 = {(uint32_t *)aes_ecb_ky_2,
                                           (uint32_t)sizeof(aes_ecb_ky_2) >> 1,
                                           (uint32_t *)aes_ecb_pt_2,
                                           (uint32_t)sizeof(aes_ecb_pt_2) >> 1,
                                           (uint32_t *)0,
                                           0,
                                           (uint32_t *)aes_ecb_ct_2,
                                           AES_192,
                                           AES_ECB};

const SEA_AES_OTHERS_TESTLIST aes_ecb_3 = {(uint32_t *)aes_ecb_ky_3,
                                           (uint32_t)sizeof(aes_ecb_ky_3) >> 1,
                                           (uint32_t *)aes_ecb_pt_3,
                                           (uint32_t)sizeof(aes_ecb_pt_3) >> 1,
                                           (uint32_t *)0,
                                           0,
                                           (uint32_t *)aes_ecb_ct_3,
                                           AES_256,
                                           AES_ECB};
/* AES CBC */
const SEA_AES_OTHERS_TESTLIST aes_cbc_0 = {(uint32_t *)aes_cbc_ky_0,
                                           (uint32_t)sizeof(aes_cbc_ky_0) >> 1,
                                           (uint32_t *)aes_cbc_pt_0,
                                           (uint32_t)sizeof(aes_cbc_pt_0) >> 1,
                                           (uint32_t *)aes_cbc_iv_0,
                                           (uint32_t)sizeof(aes_cbc_iv_0) >> 1,
                                           (uint32_t *)aes_cbc_ct_0,
                                           AES_128,
                                           AES_CBC};

const SEA_AES_OTHERS_TESTLIST aes_cbc_1 = {(uint32_t *)aes_cbc_ky_1,
                                           (uint32_t)sizeof(aes_cbc_ky_1) >> 1,
                                           (uint32_t *)aes_cbc_pt_1,
                                           (uint32_t)sizeof(aes_cbc_pt_1) >> 1,
                                           (uint32_t *)aes_cbc_iv_1,
                                           (uint32_t)sizeof(aes_cbc_iv_1) >> 1,
                                           (uint32_t *)aes_cbc_ct_1,
                                           AES_256,
                                           AES_CBC};

const SEA_AES_OTHERS_TESTLIST aes_cbc_2 = {(uint32_t *)aes_cbc_ky_2,
                                           (uint32_t)sizeof(aes_cbc_ky_2) >> 1,
                                           (uint32_t *)aes_cbc_pt_2,
                                           (uint32_t)sizeof(aes_cbc_pt_2) >> 1,
                                           (uint32_t *)aes_cbc_iv_2,
                                           (uint32_t)sizeof(aes_cbc_iv_2) >> 1,
                                           (uint32_t *)aes_cbc_ct_2,
                                           AES_192,
                                           AES_CBC};

const SEA_AES_OTHERS_TESTLIST aes_cbc_3 = {(uint32_t *)aes_cbc_ky_3,
                                           (uint32_t)sizeof(aes_cbc_ky_3) >> 1,
                                           (uint32_t *)aes_cbc_pt_3,
                                           (uint32_t)sizeof(aes_cbc_pt_3) >> 1,
                                           (uint32_t *)aes_cbc_iv_3,
                                           (uint32_t)sizeof(aes_cbc_iv_3) >> 1,
                                           (uint32_t *)aes_cbc_ct_3,
                                           AES_256,
                                           AES_CBC};

/* AES CTR */
const SEA_AES_OTHERS_TESTLIST aes_ctr_0 = {(uint32_t *)aes_ctr_ky_0,
                                           (uint32_t)sizeof(aes_ctr_ky_0) >> 1,
                                           (uint32_t *)aes_ctr_pt_0,
                                           (uint32_t)sizeof(aes_ctr_pt_0) >> 1,
                                           (uint32_t *)aes_ctr_iv_0,
                                           (uint32_t)sizeof(aes_ctr_iv_0) >> 1,
                                           (uint32_t *)aes_ctr_ct_0,
                                           AES_128,
                                           AES_CTR};

const SEA_AES_OTHERS_TESTLIST aes_ctr_1 = {(uint32_t *)aes_ctr_ky_1,
                                           (uint32_t)sizeof(aes_ctr_ky_1) >> 1,
                                           (uint32_t *)aes_ctr_pt_1,
                                           (uint32_t)sizeof(aes_ctr_pt_1) >> 1,
                                           (uint32_t *)aes_ctr_iv_1,
                                           (uint32_t)sizeof(aes_ctr_iv_1) >> 1,
                                           (uint32_t *)aes_ctr_ct_1,
                                           AES_192,
                                           AES_CTR};

const SEA_AES_OTHERS_TESTLIST aes_ctr_2 = {(uint32_t *)aes_ctr_ky_2,
                                           (uint32_t)sizeof(aes_ctr_ky_2) >> 1,
                                           (uint32_t *)aes_ctr_pt_2,
                                           (uint32_t)sizeof(aes_ctr_pt_2) >> 1,
                                           (uint32_t *)aes_ctr_iv_2,
                                           (uint32_t)sizeof(aes_ctr_iv_2) >> 1,
                                           (uint32_t *)aes_ctr_ct_2,
                                           AES_256,
                                           AES_CTR};

const SEA_AES_OTHERS_TESTLIST aes_ctr_3 = {(uint32_t *)aes_ctr_ky_3,
                                           (uint32_t)sizeof(aes_ctr_ky_3) >> 1,
                                           (uint32_t *)aes_ctr_pt_3,
                                           (uint32_t)sizeof(aes_ctr_pt_3) >> 1,
                                           (uint32_t *)aes_ctr_iv_3,
                                           (uint32_t)sizeof(aes_ctr_iv_3) >> 1,
                                           (uint32_t *)aes_ctr_ct_3,
                                           AES_256,
                                           AES_CTR};

const SEA_AES_OTHERS_TESTLIST aes_ctr_4 = {(uint32_t *)aes_ctr_ky_4,
                                           (uint32_t)sizeof(aes_ctr_ky_4) >> 1,
                                           (uint32_t *)aes_ctr_pt_4,
                                           (uint32_t)sizeof(aes_ctr_pt_4) >> 1,
                                           (uint32_t *)aes_ctr_iv_4,
                                           (uint32_t)sizeof(aes_ctr_iv_4) >> 1,
                                           (uint32_t *)aes_ctr_ct_4,
                                           AES_256,
                                           AES_CTR};

/* AES CFB128 */
const SEA_AES_OTHERS_TESTLIST aes_cfb128_0 = {(uint32_t *)aes_cfb128_ky_0,
                                              (uint32_t)sizeof(aes_cfb128_ky_0) >> 1,
                                              (uint32_t *)aes_cfb128_pt_0,
                                              (uint32_t)sizeof(aes_cfb128_pt_0) >> 1,
                                              (uint32_t *)aes_cfb128_iv_0,
                                              (uint32_t)sizeof(aes_cfb128_iv_0) >> 1,
                                              (uint32_t *)aes_cfb128_ct_0,
                                              AES_128,
                                              AES_CFB128};

const SEA_AES_OTHERS_TESTLIST aes_cfb128_1 = {(uint32_t *)aes_cfb128_ky_1,
                                              (uint32_t)sizeof(aes_cfb128_ky_1) >> 1,
                                              (uint32_t *)aes_cfb128_pt_1,
                                              (uint32_t)sizeof(aes_cfb128_pt_1) >> 1,
                                              (uint32_t *)aes_cfb128_iv_1,
                                              (uint32_t)sizeof(aes_cfb128_iv_1) >> 1,
                                              (uint32_t *)aes_cfb128_ct_1,
                                              AES_192,
                                              AES_CFB128};

const SEA_AES_OTHERS_TESTLIST aes_cfb128_2 = {(uint32_t *)aes_cfb128_ky_2,
                                              (uint32_t)sizeof(aes_cfb128_ky_2) >> 1,
                                              (uint32_t *)aes_cfb128_pt_2,
                                              (uint32_t)sizeof(aes_cfb128_pt_2) >> 1,
                                              (uint32_t *)aes_cfb128_iv_2,
                                              (uint32_t)sizeof(aes_cfb128_iv_2) >> 1,
                                              (uint32_t *)aes_cfb128_ct_2,
                                              AES_256,
                                              AES_CFB128};

const SEA_AES_OTHERS_TESTLIST aes_cfb128_3 = {(uint32_t *)aes_cfb128_ky_3,
                                              (uint32_t)sizeof(aes_cfb128_ky_3) >> 1,
                                              (uint32_t *)aes_cfb128_pt_3,
                                              (uint32_t)sizeof(aes_cfb128_pt_3) >> 1,
                                              (uint32_t *)aes_cfb128_iv_3,
                                              (uint32_t)sizeof(aes_cfb128_iv_3) >> 1,
                                              (uint32_t *)aes_cfb128_ct_3,
                                              AES_256,
                                              AES_CFB128};

/* AES OFB */
const SEA_AES_OTHERS_TESTLIST aes_ofb_0 = {(uint32_t *)aes_ofb_ky_0,
                                           (uint32_t)sizeof(aes_ofb_ky_0) >> 1,
                                           (uint32_t *)aes_ofb_pt_0,
                                           (uint32_t)sizeof(aes_ofb_pt_0) >> 1,
                                           (uint32_t *)aes_ofb_iv_0,
                                           (uint32_t)sizeof(aes_ofb_iv_0) >> 1,
                                           (uint32_t *)aes_ofb_ct_0,
                                           AES_128,
                                           AES_OFB};

const SEA_AES_OTHERS_TESTLIST aes_ofb_1 = {(uint32_t *)aes_ofb_ky_1,
                                           (uint32_t)sizeof(aes_ofb_ky_1) >> 1,
                                           (uint32_t *)aes_ofb_pt_1,
                                           (uint32_t)sizeof(aes_ofb_pt_1) >> 1,
                                           (uint32_t *)aes_ofb_iv_1,
                                           (uint32_t)sizeof(aes_ofb_iv_1) >> 1,
                                           (uint32_t *)aes_ofb_ct_1,
                                           AES_192,
                                           AES_OFB};

const SEA_AES_OTHERS_TESTLIST aes_ofb_2 = {(uint32_t *)aes_ofb_ky_2,
                                           (uint32_t)sizeof(aes_ofb_ky_2) >> 1,
                                           (uint32_t *)aes_ofb_pt_2,
                                           (uint32_t)sizeof(aes_ofb_pt_2) >> 1,
                                           (uint32_t *)aes_ofb_iv_2,
                                           (uint32_t)sizeof(aes_ofb_iv_2) >> 1,
                                           (uint32_t *)aes_ofb_ct_2,
                                           AES_192,
                                           AES_OFB};

const SEA_AES_OTHERS_TESTLIST aes_ofb_3 = {(uint32_t *)aes_ofb_ky_3,
                                           (uint32_t)sizeof(aes_ofb_ky_3) >> 1,
                                           (uint32_t *)aes_ofb_pt_3,
                                           (uint32_t)sizeof(aes_ofb_pt_3) >> 1,
                                           (uint32_t *)aes_ofb_iv_3,
                                           (uint32_t)sizeof(aes_ofb_iv_3) >> 1,
                                           (uint32_t *)aes_ofb_ct_3,
                                           AES_256,
                                           AES_OFB};

const SEA_AES_OTHERS_TESTLIST *others_testlists[] = {
    /* AES ECB */
    (SEA_AES_OTHERS_TESTLIST *)&aes_ecb_0,
    (SEA_AES_OTHERS_TESTLIST *)&aes_ecb_1,
    (SEA_AES_OTHERS_TESTLIST *)&aes_ecb_2,
    (SEA_AES_OTHERS_TESTLIST *)&aes_ecb_3,
    /* AES CBC */
    (SEA_AES_OTHERS_TESTLIST *)&aes_cbc_0,
    (SEA_AES_OTHERS_TESTLIST *)&aes_cbc_1,
    (SEA_AES_OTHERS_TESTLIST *)&aes_cbc_2,
    (SEA_AES_OTHERS_TESTLIST *)&aes_cbc_3,
    /* AES CTR */
    (SEA_AES_OTHERS_TESTLIST *)&aes_ctr_0,
    (SEA_AES_OTHERS_TESTLIST *)&aes_ctr_1,
    (SEA_AES_OTHERS_TESTLIST *)&aes_ctr_2,
    (SEA_AES_OTHERS_TESTLIST *)&aes_ctr_3,
    (SEA_AES_OTHERS_TESTLIST *)&aes_ctr_4,
    /* AES CFB128 */
    (SEA_AES_OTHERS_TESTLIST *)&aes_cfb128_0,
    (SEA_AES_OTHERS_TESTLIST *)&aes_cfb128_1,
    (SEA_AES_OTHERS_TESTLIST *)&aes_cfb128_2,
    (SEA_AES_OTHERS_TESTLIST *)&aes_cfb128_3,
    /* AES OFB */
    (SEA_AES_OTHERS_TESTLIST *)&aes_ofb_0,
    (SEA_AES_OTHERS_TESTLIST *)&aes_ofb_1,
    (SEA_AES_OTHERS_TESTLIST *)&aes_ofb_2,
    (SEA_AES_OTHERS_TESTLIST *)&aes_ofb_3,
};

#define AES_OTHERS_TESTTIMES (sizeof(others_testlists) / sizeof(others_testlists[0]))

static uint32_t aes_other_testunit(SEA_AES_OTHERS_TESTLIST *testList, uint32_t buff_size, AES_ENCRYPT_SEL encrypt_sel) {
    uint32_t i, tmp_err = SEA_SUCCESS;
    uint32_t plainData[513] = {0};
    uint32_t cryptData[513] = {0};
    uint32_t verifyData[513] = {0};
    uint32_t key[32] = {0};
    uint32_t iv[32] = {0};

    uint32_t loops, calc_data_size, ret_data_size, last_data_size;
    aesProcessParameters pParas;
    SEA_AES_OTHERS_TESTLIST *pCase = testList;
#ifdef AES_PROCESS_LATCH_RECOVER_SUPPORT
    aesProcessParameters tmp_latch_buf;
#endif

    if (buff_size > sizeof(plainData)) {
        printf("[log]++stack overflow Fail!!!\n");
        return SEA_ERROR;
    }
    str2hex((const uint8_t *)pCase->pt, (uint8_t *)plainData, pCase->ptlen);
    str2hex((const uint8_t *)pCase->key, (uint8_t *)key, pCase->keylen);
    str2hex((const uint8_t *)pCase->iv, (uint8_t *)iv, pCase->ivlen);
    str2hex((const uint8_t *)pCase->ct, (uint8_t *)cryptData, pCase->ptlen);
    AES_InitTypeDef cfg;
    cfg.encrypt_sel = encrypt_sel;
    cfg.bitWideMode = pCase->bitWideMode;
    cfg.cryptMode = pCase->cryptMode;
    cfg.keyLocation = (aesKey_t *)&key[0];
    cfg.ivLocation = (aesIV_t *)&iv[0];
#ifndef SEA_HAL_FULL_TEST
    cfg.assignedAddress = (uint32_t)&AES_GCM_DMA_TEST[0];
#else
    cfg.runningMode = AES_GENERAL_MODE_EN;
#endif
    tmp_err = LL_AES_crypt_init(&pParas, (const AES_InitTypeDef *)&cfg);
    TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);

    ret_data_size = pCase->ptlen;
    loops = ((ret_data_size % buff_size) != 0U) ? ((ret_data_size / buff_size) + 1U) : (ret_data_size / buff_size);
    for (i = 0; i < loops; i++) {
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
        calc_data_size = ((ret_data_size > buff_size) ? buff_size : ret_data_size);
        ret_data_size -= calc_data_size;
        if (encrypt_sel == AES_enCrypt) {
            tmp_err = LL_AES_crypt_sign(&pParas, \
                (const PlainData_t *)((uint32_t)plainData + (i * buff_size)), \
                calc_data_size, CypherDatasNotTheLast, (CryptData_t *)((uint32_t)verifyData + (i * buff_size)));
        } else {
            tmp_err = LL_AES_decrypt_verify(&pParas, \
                (const CryptData_t *)((uint32_t)cryptData + (i * buff_size)), \
                calc_data_size, CypherDatasNotTheLast, (PlainData_t *)((uint32_t)verifyData + (i * buff_size)));
        }
        TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);
    }
    (void)LL_AES_crypt_deinit();

    if (encrypt_sel == AES_enCrypt) {
        tmp_err = memcmp((const uint8_t *)cryptData, (const uint8_t *)verifyData, pCase->ptlen);
        if (0 != tmp_err) {
            printf("[log]++ct Check Fail!!!\n");
            printf("[log]++cryptData\tverifyData\n");
            for (int j = 0; j < (pCase->ptlen >> 2U); j++) {
                printf("\t%08X %08X\n", cryptData[j], verifyData[j]);
            }
        }
        TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);
    } else {
        tmp_err = memcmp((const uint8_t *)plainData, (const uint8_t *)verifyData, pCase->ptlen);
        if (0 != tmp_err) {
            printf("[log]++pt Check Fail!!!\n");
            printf("[log]++plainData\tverifyData\n");
            for (int j = 0; j < (pCase->ptlen >> 2U); j++) {
                printf("\t%08X %08X\n", plainData[j], verifyData[j]);
            }
        }
        TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);
    }

    return tmp_err;
}

void aes_others_test(void) {
    uint32_t i, j, tmp_err = SEA_SUCCESS;
    AES_KernelTypeDef version = ((sea_update_stat()) ? AES_RUNNING_IN_CORE0 : AES_RUNNING_IN_CORE1);
    SEA_AES_OTHERS_TESTLIST *pCase;

#ifdef AES_OTHERS_CRYPT_TEST_ENABLE
    sea_init_all(version);
    for (i = 0; i < AES_OTHERS_TESTTIMES; i++) {
        WDT_FEED;
        pCase = (SEA_AES_OTHERS_TESTLIST *)others_testlists[i];
        printf("</***********%7s %7s encrypt test start***********\n", aes_bitwide_str[pCase->bitWideMode], aes_mod_str[pCase->cryptMode]);
        for (j = 0; j < (uint32_t)(sizeof(others_buffsize_testlist) >> 2U); j++) { /* 通过开辟不同大小的内存，测试函数接口的单次和多次调用结果 */
            WDT_FEED;
            printf("[log]++list: %d max_buffer_size: %d\n", i, others_buffsize_testlist[j]);
            tmp_err = aes_other_testunit((SEA_AES_OTHERS_TESTLIST *)others_testlists[i], others_buffsize_testlist[j], AES_enCrypt);
            TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);
        }
        printf("***********%7s %7s encrypt test Pass***********/>\n", aes_bitwide_str[pCase->bitWideMode], aes_mod_str[pCase->cryptMode]);
    }
#endif

#ifdef AES_OTHERS_DECRYPT_TEST_ENABLE
    sea_init_all(version);
    for (i = 0; i < AES_OTHERS_TESTTIMES; i++) {
        WDT_FEED;
        pCase = (SEA_AES_OTHERS_TESTLIST *)others_testlists[i];
        printf("</***********%7s %7s decrypt test start***********\n", aes_bitwide_str[pCase->bitWideMode], aes_mod_str[pCase->cryptMode]);
        for (j = 0; j < (uint32_t)(sizeof(others_buffsize_testlist) >> 2U); j++) {
            WDT_FEED;
            printf("[log]++list: %d max_buffer_size: %d\n", i, others_buffsize_testlist[j]);
            tmp_err = aes_other_testunit((SEA_AES_OTHERS_TESTLIST *)others_testlists[i], others_buffsize_testlist[j], AES_deCrypt);
            TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);
        }
        printf("***********%7s %7s decrypt test Pass***********/>\n", aes_bitwide_str[pCase->bitWideMode], aes_mod_str[pCase->cryptMode]);
    }
#endif
}
