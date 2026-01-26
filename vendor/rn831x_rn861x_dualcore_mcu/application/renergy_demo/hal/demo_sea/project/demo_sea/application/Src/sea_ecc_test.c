#include "sea_test.h"
#include <stdint.h>

/******************************************************************************
 * Configuration of the Elliptic curve type
 * Allowed values:
 *     - NIST_P192 for a 192-bit ECC curve support NIST Standards
 *     - NIST_P224 for a 224-bit ECC curve support NIST Standards
 *     - NIST_P256 for a 256-bit ECC curve support NIST Standards
 *     - NIST_P384 for a 384-bit ECC curve support NIST Standards
 *     - NIST_P521 for a 521-bit ECC curve support NIST Standards
 *     - Brainpool_P256 for a 256-bit ECC curve support Brainpoolr1 Standards
 *     - Brainpool_P384 for a 384-bit ECC curve support Brainpoolr1 Standards
 *     - Brainpool_P512 for a 512-bit ECC curve support Brainpoolr1 Standards
 *****************************************************************************/
const ECC_HandleTypeDef Curve_NIST_P192 = {
    6U,   /* NumWords */
    24U,  /* NumBytes */
    192U, /* NumBits  */
    {0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFEU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U}, /* p */
    {0xB4D22831U, 0x146BC9B1U, 0x99DEF836U, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U}, /* n */
    {0xFFFFFFFCU, 0xFFFFFFFFU, 0xFFFFFFFEU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U}, /* a */
    {0xC146B9B1U, 0xFEB8DEECU, 0x72243049U, 0x0FA7E9ABU, 0xE59C80E7U, 0x64210519U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U}, /* b */
    {0x82FF1012U, 0xF4FF0AFDU, 0x43A18800U, 0x7CBF20EBU, 0xB03090F6U, 0x188DA80EU, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U,  /* Gx */
     0x1E794811U, 0x73F977A1U, 0x6B24CDD5U, 0x631011EDU, 0xFFC8DA78U, 0x07192B95U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U}, /* Gy */
};

const ECC_HandleTypeDef Curve_NIST_P224 = {
    7U,   /* NumWords */
    28U,  /* NumBytes */
    224U, /* NumBits  */
    {0x00000001U, 0x00000000U, 0x00000000U, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U}, /* p */
    {0x5C5C2A3DU, 0x13DD2945U, 0xE0B8F03EU, 0xFFFF16A2U, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U}, /* n */
    {0xFFFFFFFEU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFEU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U}, /* a */
    {0x2355FFB4U, 0x270B3943U, 0xD7BFD8BAU, 0x5044B0B7U, 0xF5413256U, 0x0C04B3ABU, 0xB4050A85U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U}, /* b */
    {0x115C1D21U, 0x343280D6U, 0x56C21122U, 0x4A03C1D3U, 0x321390B9U, 0x6BB4BF7FU, 0xB70E0CBDU, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U,  /* Gx */
     0x85007E34U, 0x44D58199U, 0x5A074764U, 0xCD4375A0U, 0x4C22DFE6U, 0xB5F723FBU, 0xBD376388U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U}, /* Gy */
};

const ECC_HandleTypeDef Curve_NIST_P256 = {
    8U,   /* NumWords */
    32U,  /* NumBytes */
    256U, /* NumBits  */
    {0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000001U, 0xFFFFFFFFU, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U}, /* p */
    {0xFC632551U, 0xF3B9CAC2U, 0xA7179E84U, 0xBCE6FAADU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0x00000000U, 0xFFFFFFFFU, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U}, /* n */
    {0xFFFFFFFCU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000001U, 0xFFFFFFFFU, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U}, /* a */
    {0x27D2604BU, 0x3BCE3C3EU, 0xCC53B0F6U, 0x651D06B0U, 0x769886BCU, 0xB3EBBD55U, 0xAA3A93E7U, 0x5AC635D8U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U}, /* b */
    {0xD898C296U, 0xF4A13945U, 0x2DEB33A0U, 0x77037D81U, 0x63A440F2U, 0xF8BCE6E5U, 0xE12C4247U, 0x6B17D1F2U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U,  /* Gx */
     0x37BF51F5U, 0xCBB64068U, 0x6B315ECEU, 0x2BCE3357U, 0x7C0F9E16U, 0x8EE7EB4AU, 0xFE1A7F9BU, 0x4FE342E2U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U}, /* Gy */
};

const ECC_HandleTypeDef Curve_NIST_P384 = {
    12U,  /* NumWords */
    48U,  /* NumBytes */
    384U, /* NumBits  */
    {0xFFFFFFFFU, 0x00000000U, 0x00000000U, 0xFFFFFFFFU, 0xFFFFFFFEU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0U, 0U, 0U, 0U, 0U}, /* p */
    {0xCCC52973U, 0xECEC196AU, 0x48B0A77AU, 0x581A0DB2U, 0xF4372DDFU, 0xC7634D81U, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0U, 0U, 0U, 0U, 0U}, /* n */
    {0xFFFFFFFCU, 0x00000000U, 0x00000000U, 0xFFFFFFFFU, 0xFFFFFFFEU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0U, 0U, 0U, 0U, 0U}, /* a */
    {0xD3EC2AEFU, 0x2A85C8EDU, 0x8A2ED19DU, 0xC656398DU, 0x5013875AU, 0x0314088FU, 0xFE814112U, 0x181D9C6EU, 0xE3F82D19U, 0x988E056BU, 0xE23EE7E4U, 0xB3312FA7U, 0U, 0U, 0U, 0U, 0U}, /* b */
    {0x72760AB7U, 0x3A545E38U, 0xBF55296CU, 0x5502F25DU, 0x82542A38U, 0x59F741E0,  0x8BA79B98U, 0x6E1D3B62U, 0xF320AD74U, 0x8EB1C71EU, 0xBE8B0537U, 0xAA87CA22U, 0U, 0U, 0U, 0U, 0U,  /* Gx */
     0x90EA0E5FU, 0x7A431D7CU, 0x1D7E819DU, 0x0A60B1CEU, 0xB5F0B8C0U, 0xE9DA3113U, 0x289A147CU, 0xF8F41DBDU, 0x9292DC29U, 0x5D9E98BFU, 0x96262C6FU, 0x3617DE4AU, 0U, 0U, 0U, 0U, 0U}, /* Gy */
};

const ECC_HandleTypeDef Curve_NIST_P521 = {
    17U,  /* NumWords */
    66U,  /* NumBytes */
    521U, /* NumBits  */
    {0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0x000001FFU}, /* p */
    {0x91386409U, 0xBB6FB71EU, 0x899C47AEU, 0x3BB5C9B8U, 0xF709A5D0U, 0x7FCC0148U, 0xBF2F966BU, 0x51868783U, 0xFFFFFFFAU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0x000001FFU}, /* n */
    {0xFFFFFFFCU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0x000001FFU}, /* a */
    {0x6B503F00U, 0xEF451FD4U, 0x3D2C34F1U, 0x3573DF88U, 0x3BB1BF07U, 0x1652C0BDU, 0xEC7E937BU, 0x56193951U, 0x8EF109E1U, 0xB8B48991U, 0x99B315F3U, 0xA2DA725BU, 0xB68540EEU, 0x929A21A0U, 0x8E1C9A1FU, 0x953EB961U, 0x00000051U}, /* b */
    {0xC2E5BD66U, 0xF97E7E31U, 0x856A429BU, 0x3348B3C1U, 0xA2FFA8DEU, 0xFE1DC127U, 0xEFE75928U, 0xA14B5E77U, 0x6B4D3DBAU, 0xF828AF60U, 0x053FB521U, 0x9C648139U, 0x2395B442U, 0x9E3ECB66U, 0x0404E9CDU, 0x858E06B7U, 0x000000C6U,  /* Gx */
     0x9FD16650U, 0x88BE9476U, 0xA272C240U, 0x353C7086U, 0x3FAD0761U, 0xC550B901U, 0x5EF42640U, 0x97EE7299U, 0x273E662CU, 0x17AFBD17U, 0x579B4468U, 0x98F54449U, 0x2C7D1BD9U, 0x5C8A5FB4U, 0x9A3BC004U, 0x39296A78U, 0x00000118U}, /* Gy */
};

const ECC_HandleTypeDef Curve_Brainpool_256 = {
    8U,   /* NumWords */
    32U,  /* NumBytes */
    256U, /* NumBits  */
    {0x1F6E5377U, 0x2013481DU, 0xD5262028U, 0x6E3BF623U, 0x9D838D72U, 0x3E660A90U, 0xA1EEA9BCU, 0xA9FB57DBU, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U}, /* p */
    {0x974856A7U, 0x901E0E82U, 0xB561A6F7U, 0x8C397AA3U, 0x9D838D71U, 0x3E660A90U, 0xA1EEA9BCU, 0xA9FB57DBU, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U}, /* n */
    {0xF330B5D9U, 0xE94A4B44U, 0x26DC5C6CU, 0xFB8055C1U, 0x417AFFE7U, 0xEEF67530U, 0xFC2C3057U, 0x7D5A0975U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U}, /* a */
    {0xFF8C07B6U, 0x6BCCDC18U, 0x5CF7E1CEU, 0x95841629U, 0xBBD77CBFU, 0xF330B5D9U, 0xE94A4B44U, 0x26DC5C6CU, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U}, /* b */
    {0x9ACE3262U, 0x3A4453BDU, 0xE3BD23C2U, 0xB9DE27E1U, 0xFC81B7AFU, 0x2C4B482FU, 0xCB7E57CBU, 0x8BD2AEB9U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U,  /* Gx */
     0x2F046997U, 0x5C1D54C7U, 0x2DED8E54U, 0xC2774513U, 0x14611DC9U, 0x97F8461AU, 0xC3DAC4FDU, 0x547EF835U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U}, /* Gy */
};

const ECC_HandleTypeDef Curve_Brainpool_384 = {
    12U,  /* NumWords */
    48U,  /* NumBytes */
    384U, /* NumBits  */
    {0x3107EC53U, 0x87470013U, 0x901D1A71U, 0xACD3A729U, 0x7FB71123U, 0x12B1DA19U, 0xED5456B4U, 0x152F7109U, 0x50E641DFU, 0x0F5D6F7EU, 0xA3386D28U, 0x8CB91E82U, 0U, 0U, 0U, 0U, 0U}, /* p */
    {0xE9046565U, 0x3B883202U, 0x6B7FC310U, 0xCF3AB6AFU, 0xAC0425A7U, 0x1F166E6CU, 0xED5456B3U, 0x152F7109U, 0x50E641DFU, 0x0F5D6F7EU, 0xA3386D28U, 0x8CB91E82U, 0U, 0U, 0U, 0U, 0U}, /* n */
    {0x22CE2826U, 0x04A8C7DDU, 0x503AD4EBU, 0x8AA5814AU, 0xBA91F90FU, 0x139165EFU, 0x4FB22787U, 0xC2BEA28EU, 0xCE05AFA0U, 0x3C72080AU, 0x3D8C150CU, 0x7BC382C6U, 0U, 0U, 0U, 0U, 0U}, /* a */
    {0xFA504C11U, 0x3AB78696U, 0x95DBC994U, 0x7CB43902U, 0x3EEB62D5U, 0x2E880EA5U, 0x07DCD2A6U, 0x2FB77DE1U, 0x16F0447CU, 0x8B39B554U, 0x22CE2826U, 0x04A8C7DDU, 0U, 0U, 0U, 0U, 0U}, /* b */
    {0x47D4AF1EU, 0xEF87B2E2U, 0x36D646AAU, 0xE826E034U, 0x0CBD10E8U, 0xDB7FCAFEU, 0x7EF14FE3U, 0x8847A3E7U, 0xB7C13F6BU, 0xA2A63A81U, 0x68CF45FFU, 0x1D1C64F0U, 0U, 0U, 0U, 0U, 0U,  /* Gx */
     0x263C5315U, 0x42820341U, 0x77918111U, 0x0E464621U, 0xF9912928U, 0xE19C054FU, 0xFEEC5864U, 0x62B70B29U, 0x95CFD552U, 0x5CB1EB8EU, 0x20F9C2A4U, 0x8ABE1D75U, 0U, 0U, 0U, 0U, 0U}, /* Gy */
};

const ECC_HandleTypeDef Curve_Brainpool_512 = {
    16U,  /* NumWords */
    64U,  /* NumBytes */
    512U, /* NumBits  */
    {0x583A48F3U, 0x28AA6056U, 0x2D82C685U, 0x2881FF2FU, 0xE6A380E6U, 0xAECDA12AU, 0x9BC66842U, 0x7D4D9B00U, 0x70330871U, 0xD6639CCAU, 0xB3C9D20EU, 0xCB308DB3U, 0x33C9FC07U, 0x3FD4E6AEU, 0xDBE9C48BU, 0xAADD9DB8U, 0U}, /* p */
    {0x9CA90069U, 0xB5879682U, 0x085DDADDU, 0x1DB1D381U, 0x7FAC1047U, 0x41866119U, 0x4CA92619U, 0x553E5C41U, 0x70330870U, 0xD6639CCAU, 0xB3C9D20EU, 0xCB308DB3U, 0x33C9FC07U, 0x3FD4E6AEU, 0xDBE9C48BU, 0xAADD9DB8U, 0U}, /* n */
    {0x77FC94CAU, 0xE7C1AC4DU, 0x2BF2C7B9U, 0x7F1117A7U, 0x8B9AC8B5U, 0x0A2EF1C9U, 0xA8253AA1U, 0x2DED5D5AU, 0xEA9863BCU, 0xA83441CAU, 0x3DF91610U, 0x94CBDD8DU, 0xAC234CC5U, 0xE2327145U, 0x8B603B89U, 0x7830A331U, 0U}, /* a */
    {0x8016F723U, 0x2809BD63U, 0x5EBAE5DDU, 0x984050B7U, 0xDC083E67U, 0x77FC94CAU, 0xE7C1AC4DU, 0x2BF2C7B9U, 0x7F1117A7U, 0x8B9AC8B5U, 0x0A2EF1C9U, 0xA8253AA1U, 0x2DED5D5AU, 0xEA9863BCU, 0xA83441CAU, 0x3DF91610U, 0U}, /* b */
    {0xBCB9F822U, 0x8B352209U, 0x406A5E68U, 0x7C6D5047U, 0x93B97D5FU, 0x50D1687BU, 0xE2D0D48DU, 0xFF3B1F78U, 0xF4D0098EU, 0xB43B62EEU, 0xB5D916C1U, 0x85ED9F70U, 0x9C4C6A93U, 0x5A21322EU, 0xD82ED964U, 0x81AEE4BDU, 0U,  /* Gx */
     0x3AD80892U, 0x78CD1E0FU, 0xA8F05406U, 0xD1CA2B2FU, 0x8A2763AEU, 0x5BCA4BD8U, 0x4A5F485EU, 0xB2DCDE49U, 0x881F8111U, 0xA000C55BU, 0x24A57B1AU, 0xF209F700U, 0xCF7822FDU, 0xC0EABFA9U, 0x566332ECU, 0x7DDE385DU, 0U}, /* Gy */
};

static void hash_once(HASH_HandleTypeDef *pParas, const uint8_t *message, uint32_t msglen, const HASH_InitTypeDef *sha_cfg, uint32_t *Hm, uint32_t Hm_len) {
    uint32_t tmp_data[512];
    int cnt;
    uint8_t *pHm = (uint8_t *)Hm;
    uint8_t temp;
    uint32_t tmp_err = SEA_SUCCESS;

    /******************initial hash function paramter*************************/
    tmp_err = LL_HASH_init(pParas, sha_cfg);
    TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);

    str2hex((const uint8_t *)message, (uint8_t *)tmp_data, msglen);
    /*******************caculate the value of hash****************************/
    tmp_err = LL_HASH(pParas, (const hashData_t *)(tmp_data), msglen, CypherDatasIsTheLast, (hashData_t *)Hm);
    TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);
    (void)LL_HASH_deinit();

    /*****************output hash results in reverse byte order***************/
    for (cnt = 0; cnt < (Hm_len >> 1); cnt++) {
        temp = pHm[cnt];
        pHm[cnt] = pHm[Hm_len - 1 - cnt];
        pHm[Hm_len - 1 - cnt] = temp;
    }
}

uint32_t startTime;
uint32_t endTime;

const uint8_t message_1216[] = {
    "910D172CC05D587B6D8B5BE4FC0C54A1EABB70C8D2BC46AA050394C3BB9CD14B37BAB1BF59E341FA270FF88A54AC6465B334E070C79BED42CD710F02587F8B6615B090617208D6CB2C3863C0FF131C1505EC434602"
    "E7FBC81B23C730AE6D328BCFF79D71C16A52F77BE258C363096694A1BE6F8B0A5A5DB32152CBC4FF51AE84FCC7CA66A8BED474774EA0609B1C1AE7E8C7815CF12E52995D462F96DEC9451C43F3E07EF04F5C231B60"
    "B95DF4BBF950E8AA8E456DF177DC45BBB504E1EFD531AC951B956D0291760CEF4E680B9855134A1185B4FBF2B15E884D8081C5681269C9C5F649CE44616D5E89371F50FF29C63D7B6DF4599FF6A2DFEAD9CF70EFFF"
    "440E9E68355F928EB705DE53F1949149621CF171C5AC200832FA56DB5DFC674CB17ACCB0996146B8AE41191561D1F0F9BB47D536BDA17A10BF37CA7B1840CFD11A4025E28E4D913753AF1240BDE040FD756E1C2CCD"
    "828CD75D5AFD2A999967C181D7F5A1CE6667D874A3B84A78D76BBD6264157F0151A910E1F448F52505DB0BF98B18289DA2618AED1E7D77926ECBA550232F75A5604C9AFF924330F155F6F11A4AB8437DBBE092D9BA"
    "E04DEBA8A325243A4A1BF7F656B1CE96E37D343B45CE6C681F3593EFDF9A3E266CFB121B30E310786B3623625A3A804B335D561FB946B5F2681B8CB39E8F706E7D5131D6FC02F3399CEE81FAC1D584ACE9317F5009"
    "BC789DD20F1D241DAD66EC30E537DFE7697CB2AD5FDC31E48702863FA960057CA153B530184079B4A199781F6582C35BC4554936FCF15D72A9B61288D1C6B6268652B2A892A33CB3B3AE9FBC4EDA9A998A57065F87"
    "8BCF54FC48D455F32C634155C7FA001B4EB2CAB5D1C2CB4559BA7D0B05B534070CED8152CEF6456EF284757AE5160858A69F408C0D6C740D2222F38D424778A2DD266CCF53A6BB8F3FD82145D0ED684C47BF537D57"
    "122E78BDD4F98A85844614CA45674E625017F91D989E3BE432E21BAC7912885B42E74474CE206A9FD827DD140678B20815C55947AEC03F57A247DD3F9962AD625A1A42F8706328A95FA8DDD7F5DE2C0F22B40F2674"
    "E37684DFB5D8F7C9CA48B6FFD3F23287D4BD25ECCE2A25451E0C7B3FA709E119AC364D4DCACC2D8915788ACD23F602596BB72C9529D3AB30C34FACE2610C425416D9F5D64C9E3E01534EBF795BE994CBEFDC4BE95E"
    "F7F3A20C9AEC08CE80D7D4F1E5B6B7C2F136B7A08A8D1C75B7CAB0587A6339E13439CD91B7C78255DF8BE06B35312699EC4652152D1D98C4855AD41D41E85961430A220B50A408434BE4BE2252A2D08BE7F140D4C4"
    "7CEC57451D35C00BCE3A9EAE330D05BC0E8B2D1A17911974DDE2BA1ADFEB7C8607945010DF19DBD1225BDDFE3300345CEA6BD36EC7DC59CDFC365FF806E266AC39D1AE686AF028E1F9F4F6705F16AB3DDAC1FC4698"
    "B658B6FDBB0108FBD5C518CC3C15DE3D0F1EE7B16C241F925688FE3E5ACECCFB32E94FB0EC7789D8D94D1E1388C7E714EA97A98EF879810508A37444D9E149F3D9A193242AC9D5B8ADDF2AED2A2735CBCDD720A4C2"
    "A59CC60E7979C23036B8043B483BBE92B5EC0B3C6B34943F8F7E8265A57B91E6853C145F26D916D41124E1746A7DA9A99A7148690729FEBAFCECE7904ED354182C6FB9F1E55904FD5512656AEE912C770ED038FA93"
    "BE553E2EF9B92AD76CE7768ED00596F5B77AD36BDC285D682DB3"};
// const uint8_t Hm_1216[] = {"BA62C8F48C17A444D2675696ECC01EA5F6D1EEE67186BE7BC780690271A07076"}; // correct result obtained using SHA-256

const uint8_t message_496[] = {
    "6AA81CB092076DF680B26CEDDB37E6128A152BD2884E33BD72788F3B9FC0114B7616F51F0153F6A1268335B96C0293CAFC3A6F9129165560C71E0388A9937AB56BD9F06E02E24CF9A654E9DEAEFADB760038CE1975"
    "7D256685F8024E4D9DF60F61E4B448789302CB54E40E4E7DDB58C96C51259FA3DB991F21690204ADA0DEB5A366B907C448C726D872F50F9C47CF87FCB0EF2B5299761BEC789040BADA3D49F4AB954C62557A5C7D78"
    "3919F78598B2A9BCDFEBEAD26138B31C5D3EA5449E118BED9535F97844DC9BDAFEB3D2293849B7358458ECC83689E08FC5A73215377B2C0B432FCF50E82D15CCB4215C42C879C49054DCC0CA1CCF1C6485E2A65AA8"
    "4FFB5BF69AA2FCDA0B124423089853EE66716B01DB6532D622A6E62BA95BFA7239D44C744AA83AE4C56BB90E97EDC2B5480EE1DC18937D9C6FA59ED487C145C520DB10CB4D9BFC474E84955901A27CC07A6EEDEF20"
    "1B4D4AA09A1047F088695BC4BAB013AE3D2335FD02F766DE1E367FEC8F5DF11D0A7FD1EFBF0971D1FA4A46EB747737226950D75F0BFAA0EC3D09AA0BE1413ACE54CF3CF2D37CFE158341F8A2E4A3E8C842612ECA2B"
    "46ED4772A46CBD1FF512F2ED2A8F5C407A83971A03A5F8421291A9C7C901F43427F9E32C7D6C57141819293A3AB5764DAF92435662F1252919C5DEFC80D6381E4D2CA3A1DC0BD2"};
// const uint8_t Hm_496[] = {"60F38AADB1BDBFCB26147D8C394E2DC055C7B9DB773B394E4E8D65CA5DAB235CE0E2A9749E52C29BDE2067EA5659A59F"}; // correct result obtained using SHA-384

const HASH_InitTypeDef sha1_cfg = {SEA_SHA_1, HASH_GENERAL_MODE_EN, {0, 0}};
const HASH_InitTypeDef sha224_cfg = {SEA_SHA_224, HASH_GENERAL_MODE_EN, {0, 0}};
const HASH_InitTypeDef sha256_cfg = {SEA_SHA_256, HASH_GENERAL_MODE_EN, {0, 0}};
const HASH_InitTypeDef sha384_cfg = {SEA_SHA_384, HASH_GENERAL_MODE_EN, {0, 0}};
const HASH_InitTypeDef sha512_cfg = {SEA_SHA_512, HASH_GENERAL_MODE_EN, {0, 0}};
const HASH_InitTypeDef *test_sha_list[] = {
    (HASH_InitTypeDef *)&sha1_cfg,
    (HASH_InitTypeDef *)&sha224_cfg,
    (HASH_InitTypeDef *)&sha256_cfg,
    (HASH_InitTypeDef *)&sha384_cfg,
    (HASH_InitTypeDef *)&sha512_cfg,
};
const ECC_HandleTypeDef *test_curve_list[] = {
    (ECC_HandleTypeDef *)&Curve_NIST_P192,
    (ECC_HandleTypeDef *)&Curve_NIST_P256,
    (ECC_HandleTypeDef *)&Curve_NIST_P384,
    // (ECC_HandleTypeDef *)&Curve_NIST_P521,
    (ECC_HandleTypeDef *)&Curve_Brainpool_256,
    (ECC_HandleTypeDef *)&Curve_Brainpool_384,
    // (ECC_HandleTypeDef *)&Curve_Brainpool_512,
};
const uint8_t testtime[] = {
    1,
    3,
    4,
    // 5,
    3,
    4,
    // 5,
};

/**
 * @brief sign and verify by self
 */
void ECC_sign_test(void) {
    int i;
    HAL_SEA_FlagTypeDef ver_flag = PASS;
    HASH_InitTypeDef *test_cfg;
    HASH_HandleTypeDef pParas;
    ECC_HandleTypeDef *test_curve;

    uint32_t tmp_err = SEA_SUCCESS;
    uint32_t Hm_len;
    uint32_t Hm[17];
    uint32_t _Q[34 + 1];
    uint32_t _pkey[17 + 1];
    uint32_t _r[17 + 1];
    uint32_t _s[17 + 1];
#ifdef ECC_CRYPT_ITEMS_IN_BYTES
    uint8_t *Q = (uint8_t *)((uint32_t)&_Q[0] + 1);
    uint8_t *pkey = (uint8_t *)((uint32_t)&_pkey[0] + 1);
    uint8_t *r = (uint8_t *)((uint32_t)&_r[0] + 1);
    uint8_t *s = (uint8_t *)((uint32_t)&_s[0] + 1);
#else
    uint8_t *Q = (uint8_t *)((uint32_t)&_Q[0]);
    uint8_t *pkey = (uint8_t *)((uint32_t)&_pkey[0]);
    uint8_t *r = (uint8_t *)((uint32_t)&_r[0]);
    uint8_t *s = (uint8_t *)((uint32_t)&_s[0]);
#endif
    printf("Q addr = %08x, pkey addr = %08x, r addr = %08x, s addr = %08x\n", Q, pkey, r, s);

    for (int j = 0; j < (sizeof(test_curve_list) / 4); j++) {
        printf("***********ECC sign and verify test %d start*********\n", j);
        for (int y = 0; y < testtime[j]; y++) {
            WDT_FEED;
            sea_memset_word(Hm, 0U, 17U);
            sea_memset_word(_Q, 0U, 34U);
            sea_memset_word(_pkey, 0U, 17U);
            sea_memset_word(_r, 0U, 17U);
            sea_memset_word(_s, 0U, 17U);

            /* test config */
            test_cfg = (HASH_InitTypeDef *)test_sha_list[y];
            Hm_len = getHashOutSize(test_cfg);
            test_curve = (ECC_HandleTypeDef *)test_curve_list[j];
            if (j < 4) {
                printf("------Testing NIST-P%d with SHA-%d------\n", test_curve->NumBits, (Hm_len * 8));
            } else {
                printf("------Testing Brainpool-P%d with SHA-%d------\n", test_curve->NumBits, (Hm_len * 8));
            }

            hash_once(&pParas, message_1216, (sizeof(message_1216) >> 1), test_cfg, Hm, Hm_len);
            printf("[log]step1. HASH Values Output by SHA-%d\n", (Hm_len * 8));
            printf("\tHm   \t:");
#ifdef ECC_CRYPT_ITEMS_IN_BYTES
            word2str(Hm, Hm_len);
            for (i = 0; i < Hm_len; i++)
#else
            for (i = (Hm_len - 1); i >= 0; i--)
#endif
            {
                printf("%02X", ((uint8_t *)Hm)[i]);
            }
            printf("\n");

            WDT_FEED;
            tmp_err = LL_ECC_sign_gen(test_curve, (ECCdata_t *)Hm, Hm_len, (ECCdata_t *)Q, (ECCdata_t *)pkey, (ECCdata_t *)r, (ECCdata_t *)s);
            TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);
            if (test_curve->NumBits == 521U) {
                WDT_FEED;
                tmp_err = LL_ECC_specify_pkey_sign_gen(test_curve, (ECCdata_t *)Hm, Hm_len, (ECCdata_t *)Q, (ECCdata_t *)pkey, (ECCdata_t *)r, (ECCdata_t *)s);
                TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);
            }
            if (j < 4) {
                printf("[log]step2. ECC Sign Result Output by NIST-P%d\n", test_curve->NumBits);
            } else {
                printf("[log]step2. ECC Sign Result Output by Brainpool-P%d\n", test_curve->NumBits);
            }
            printf("\tQx   \t:");
#ifdef ECC_CRYPT_ITEMS_IN_BYTES
            for (i = 0; i < test_curve->NumBytes; i++)
#else
            for (i = (test_curve->NumBytes - 1); i >= 0; i--)
#endif
            {
                printf("%02X", ((uint8_t *)Q)[i]);
            }
            printf("\n");
            printf("\tQy   \t:");
#ifdef ECC_CRYPT_ITEMS_IN_BYTES
            for (i = 0; i < test_curve->NumBytes; i++)
#else
            for (i = (test_curve->NumBytes - 1); i >= 0; i--)
#endif
            {
                printf("%02X", ((uint8_t *)Q)[i + test_curve->NumBytes]);
            }
            printf("\n");
            printf("\tpkey \t:");
#ifdef ECC_CRYPT_ITEMS_IN_BYTES
            for (i = 0; i < test_curve->NumBytes; i++)
#else
            for (i = (test_curve->NumBytes - 1); i >= 0; i--)
#endif
            {
                printf("%02X", ((uint8_t *)pkey)[i]);
            }
            printf("\n");
            printf("\tr    \t:");
#ifdef ECC_CRYPT_ITEMS_IN_BYTES
            for (i = 0; i < test_curve->NumBytes; i++)
#else
            for (i = (test_curve->NumBytes - 1); i >= 0; i--)
#endif
            {
                printf("%02X", ((uint8_t *)r)[i]);
            }
            printf("\n");
            printf("\ts    \t:");
#ifdef ECC_CRYPT_ITEMS_IN_BYTES
            for (i = 0; i < test_curve->NumBytes; i++)
#else
            for (i = (test_curve->NumBytes - 1); i >= 0; i--)
#endif
            {
                printf("%02X", ((uint8_t *)s)[i]);
            }

            tmp_err = LL_ECC_sign_ver(test_curve, (ECCdata_t *)Hm, Hm_len, (ECCdata_t *)Q, (ECCdata_t *)r, (ECCdata_t *)s, &ver_flag);
            TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);
            if (j < 4) {
                printf("[log]step3. ECC Verify Result Output by NIST-P%d\n", test_curve->NumBits);
            } else {
                printf("[log]step3. ECC Verify Result Output by Brainpool-P%d\n", test_curve->NumBits);
            }
            printf("\tVerify :\t");
            if (ver_flag != PASS) {
                printf("FAIL\n");
                return;
            }
            printf("PASS\n");
        }
        printf("***********ECC sign and verify test %d end*********\n", j);
    }
}

/**
 * @brief verify by NIST standard datas
 */
void ECDSA_Ver_NIST256(void) {
    HAL_SEA_FlagTypeDef ver_flag = PASS;
    uint32_t Hm[8] = {0};
    uint32_t Q[16] = {0};
    uint32_t r[8] = {0};
    uint32_t s[8] = {0};

    int cnt;
    uint8_t *pQx = (uint8_t *)Q;
    uint8_t *pQy = (uint8_t *)Q + Curve_NIST_P256.NumBytes;
    uint8_t *pR = (uint8_t *)r;
    uint8_t *pS = (uint8_t *)s;
    uint8_t temp;

    const uint8_t message[] = "97B70AF1C5DA8483A0040345A47DF796060E332B1C76D1EC3091633D0CB66CCE4D51494801199BD21CEDB8C9E72FAC53D271B0A13B9AA5E1000B0A60ACA07058CDEA7450D1C907"
                              "B6AF6FC6C157D1F73BF20FDD7D0F3AB8C2A11A1108D04B41E2AE0F9BDE6B9F62B0EEBFDBF45C46FB1114DAACEE831F6869C8E0EF3479F6768A";
    const uint8_t Qx[] = "20D3EDB582A7A2AD678EEAF3D6BC265DCE1D50FC2ED062B0787768989E40C488";
    const uint8_t Qy[] = "C1EE738BB1EA38E13446F63D4BA66F4B051A55F21C37D8BD48EE4DC2165D2211";
    const uint8_t R[] = "585FD4B43C2B027430E202BDFB92574166E5EA563D5174FB861C6946CA40DCC4";
    const uint8_t S[] = "956478653FDA85694A603C4557378A94CD73E261994E5FA5E162220C6FF5B5EA";

    str2hex((const uint8_t *)Qx, pQx, Curve_NIST_P256.NumBytes);
#ifndef ECC_CRYPT_ITEMS_IN_BYTES
    for (cnt = 0; cnt < (Curve_NIST_P256.NumBytes >> 1); cnt++) {
        temp = pQx[cnt];
        pQx[cnt] = pQx[Curve_NIST_P256.NumBytes - 1 - cnt];
        pQx[Curve_NIST_P256.NumBytes - 1 - cnt] = temp;
    }
#endif
    str2hex((const uint8_t *)Qy, pQy, Curve_NIST_P256.NumBytes);
#ifndef ECC_CRYPT_ITEMS_IN_BYTES
    for (cnt = 0; cnt < (Curve_NIST_P256.NumBytes >> 1); cnt++) {
        temp = pQy[cnt];
        pQy[cnt] = pQy[Curve_NIST_P256.NumBytes - 1 - cnt];
        pQy[Curve_NIST_P256.NumBytes - 1 - cnt] = temp;
    }
#endif
    str2hex((const uint8_t *)R, pR, Curve_NIST_P256.NumBytes);
#ifndef ECC_CRYPT_ITEMS_IN_BYTES
    for (cnt = 0; cnt < (Curve_NIST_P256.NumBytes >> 1); cnt++) {
        temp = pR[cnt];
        pR[cnt] = pR[Curve_NIST_P256.NumBytes - 1 - cnt];
        pR[Curve_NIST_P256.NumBytes - 1 - cnt] = temp;
    }
#endif
    str2hex((const uint8_t *)S, pS, Curve_NIST_P256.NumBytes);
#ifndef ECC_CRYPT_ITEMS_IN_BYTES
    for (cnt = 0; cnt < (Curve_NIST_P256.NumBytes >> 1); cnt++) {
        temp = pS[cnt];
        pS[cnt] = pS[Curve_NIST_P256.NumBytes - 1 - cnt];
        pS[Curve_NIST_P256.NumBytes - 1 - cnt] = temp;
    }
#endif
    WDT_FEED;
    printf("***********ECDSA NIST standard test start*********\n");
    /*************1.Calculate SHA-256 HASH Value*************/
    HASH_HandleTypeDef pParas;
    hash_once(&pParas, message, sizeof(message) >> 1, &sha256_cfg, Hm, 32);
#ifdef ECC_CRYPT_ITEMS_IN_BYTES
    word2str(Hm, Curve_NIST_P256.NumBytes);
#endif

    /*************2.ECDSA verify and result output*************/
    (void)LL_ECC_sign_ver(&Curve_NIST_P256, (ECCdata_t *)Hm, 32, (ECCdata_t *)Q, (ECCdata_t *)r, (ECCdata_t *)s, &ver_flag);

    printf("ECDSA Verify Result Output by NIST-P256\n");
    printf("\tVerify:\t:");
    if (ver_flag != PASS) {
        printf("FAIL\n");
        return;
    }
    printf("PASS\n");
    printf("***********ECDSA NIST standard test end*********\n");
}

/**
 * @brief verify by NIST standard datas
 */
void ECDSA_Ver_NIST384(void) {
    HAL_SEA_FlagTypeDef ver_flag = PASS;
    uint32_t Hm[12] = {0};
    uint32_t Q[24] = {0};
    uint32_t r[12] = {0};
    uint32_t s[12] = {0};

    int cnt;
    uint8_t *pQx = (uint8_t *)Q;
    uint8_t *pQy = (uint8_t *)Q + Curve_NIST_P384.NumBytes;
    uint8_t *pR = (uint8_t *)r;
    uint8_t *pS = (uint8_t *)s;
    uint8_t temp;

    const uint8_t message[] = "86688AC953C3CA9DE8904B7003CE1CE0EEC37146C6B5B19ED61C0E818FA59355D619579F5398A428E5AB042F4AB72E135EAF4F4CD17CD1DE07DD61F8ECAF48ECC0962731E17A8F"
                              "3E0F43C29894B97C3814FA31D79153ECFE4B3919E2D7B46BA74398E9C6F0430D8F86E1A0BD7A1C1721BF39AC57568B84A95D1875EE0A1520AF";
    const uint8_t Qx[] = "E900D4A2AD836FB4B62A5738FBB82EF598F1DC0B44F70B6F793066F0181ECFC7DE2C5F4839F6B11563CF0BC758743138";
    const uint8_t Qy[] = "9622B01B50C7AF94A296B5BB846310D7958E97FA4DE2E88CE523E9F82B061BD4F1249358DDF417558B2F9BF4EDF568E4";
    const uint8_t R[] = "8A1C4E5758F63FD9F409FC943BE6308D5ACBF67F82DE7464EBCC2ECD46A5C5754B106C6095AA90537429C058D3DE820C";
    const uint8_t S[] = "9E13CE35B511300DB9B7A96556497EC4AF983A857C36DAEF1C5DEA379F890B3F220BD1408C9A2D0BDA7DD4397C7393DE";

    str2hex((const uint8_t *)Qx, pQx, Curve_NIST_P384.NumBytes);
#ifndef ECC_CRYPT_ITEMS_IN_BYTES
    for (cnt = 0; cnt < (Curve_NIST_P384.NumBytes >> 1); cnt++) {
        temp = pQx[cnt];
        pQx[cnt] = pQx[Curve_NIST_P384.NumBytes - 1 - cnt];
        pQx[Curve_NIST_P384.NumBytes - 1 - cnt] = temp;
    }
#endif
    str2hex((const uint8_t *)Qy, pQy, Curve_NIST_P384.NumBytes);
#ifndef ECC_CRYPT_ITEMS_IN_BYTES
    for (cnt = 0; cnt < (Curve_NIST_P384.NumBytes >> 1); cnt++) {
        temp = pQy[cnt];
        pQy[cnt] = pQy[Curve_NIST_P384.NumBytes - 1 - cnt];
        pQy[Curve_NIST_P384.NumBytes - 1 - cnt] = temp;
    }
#endif
    str2hex((const uint8_t *)R, pR, Curve_NIST_P384.NumBytes);
#ifndef ECC_CRYPT_ITEMS_IN_BYTES
    for (cnt = 0; cnt < (Curve_NIST_P384.NumBytes >> 1); cnt++) {
        temp = pR[cnt];
        pR[cnt] = pR[Curve_NIST_P384.NumBytes - 1 - cnt];
        pR[Curve_NIST_P384.NumBytes - 1 - cnt] = temp;
    }
#endif
    str2hex((const uint8_t *)S, pS, Curve_NIST_P384.NumBytes);
#ifndef ECC_CRYPT_ITEMS_IN_BYTES
    for (cnt = 0; cnt < (Curve_NIST_P384.NumBytes >> 1); cnt++) {
        temp = pS[cnt];
        pS[cnt] = pS[Curve_NIST_P384.NumBytes - 1 - cnt];
        pS[Curve_NIST_P384.NumBytes - 1 - cnt] = temp;
    }
#endif
    WDT_FEED;
    printf("***********ECDSA NIST standard test start*********\n");
    /*************1.Calculate SHA-384 HASH Value*************/
    HASH_HandleTypeDef pParas;
    hash_once(&pParas, message, sizeof(message) >> 1, &sha384_cfg, Hm, 48);
#ifdef ECC_CRYPT_ITEMS_IN_BYTES
    word2str(Hm, Curve_NIST_P384.NumBytes);
#endif

    /*************2.ECDSA verify and result output*************/
    LL_ECC_sign_ver(&Curve_NIST_P384, (ECCdata_t *)Hm, 48, (ECCdata_t *)Q, (ECCdata_t *)r, (ECCdata_t *)s, &ver_flag);

    printf("ECDSA Verify Result Output by NIST-P384\n");
    printf("\tVerify:\t:");
    if (ver_flag != PASS) {
        printf("FAIL\n");
        return;
    }
    printf("PASS\n");
    printf("***********ECDSA NIST standard test end*********\n");
}

/**
 * @brief verify by NIST standard datas
 */
void ECDSA_Ver_NIST521(void) {
    HAL_SEA_FlagTypeDef ver_flag = PASS;
    uint32_t Hm[17] = {0};
    uint32_t Q[34] = {0};
    uint32_t r[17] = {0};
    uint32_t s[17] = {0};
    int cnt;
    uint32_t lentmp;

    const uint8_t message[] = "330EC16DEAC2EC1098B48DC15FB0C1C806FA5881D25C062EBD9C84AEE49DFF799AAA21C055E6A2F150C38022321654B6916CD923224EB755CE17548DB33CD419D7C0D8B3675234DF1D7A"
                              "CF89B2D546112938EAB8B6D63A735E89138AC6BF6C1F4D33F779287DEE7C2590F95CC115DEE2A11A4C1F5C214CCF39003155CE597017";
#ifndef ECC_CRYPT_ITEMS_IN_BYTES
    lentmp = 68;
    const uint8_t Qx[] = "0000001497B2C2497FD00946598802254BE4BD82A1F9EC3F45BE911EB36C87F4F7E7F660E3B2CDBEC4E8FC54A24067494E77F08A08FFF78BCD14C6BEA9163AFDF8B2020C";
    const uint8_t Qy[] = "0000005F93E5AD90637C1E7432C1C860891C6AC5F23D375B000247FB83E36C7D8BB8929F2C6A87A96D3812CB1641459EEAA1272EEA6D57375708B8766583F50ED31012A5";
    const uint8_t R[] = "000000B36F1ED6798E7D6B81ED5D84EDE12F143E05DA90BBB9DC172EDE4693D5F639D78E2F144EBF22DCE6EFA133D42C7EF4DFD862521F6B885352678E5FD60962DCC6E7";
    const uint8_t S[] = "0000007DC54288D4F979BEC21057B333E98FF7B2912915706E553CF876D10F388965A5118EFAFC08F6CD1054549EE63CA0A987C89276DF6F72C64B0AB3D6ACAB0C559333";
    // Hm:  4C26A314 64EB2C6D B29A176B DFDA07BE 0493245D 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
    // Qx:  F8B2020C A9163AFD CD14C6BE 08FFF78B 4E77F08A A2406749 C4E8FC54 E3B2CDBE F7E7F660 B36C87F4 45BE911E A1F9EC3F 4BE4BD82 59880225 7FD00946 97B2C249 00000014
    // Qy:  D31012A5 6583F50E 5708B876 EA6D5737 EAA1272E 1641459E 6D3812CB 2C6A87A9 8BB8929F 83E36C7D 000247FB F23D375B 891C6AC5 32C1C860 637C1E74 93E5AD90 0000005F
    // r :  62DCC6E7 8E5FD609 88535267 62521F6B 7EF4DFD8 A133D42C 22DCE6EF 2F144EBF F639D78E DE4693D5 B9DC172E 05DA90BB E12F143E ED5D84ED 8E7D6B81 6F1ED679 000000B3
    // s :  0C559333 B3D6ACAB 72C64B0A 9276DF6F A0A987C8 549EE63C F6CD1054 8EFAFC08 8965A511 76D10F38 6E553CF8 91291570 E98FF7B2 1057B333 F979BEC2 C54288D4 0000007D
#else
    lentmp = 66;
    const uint8_t Qx[] = "001497B2C2497FD00946598802254BE4BD82A1F9EC3F45BE911EB36C87F4F7E7F660E3B2CDBEC4E8FC54A24067494E77F08A08FFF78BCD14C6BEA9163AFDF8B2020C";
    const uint8_t Qy[] = "005F93E5AD90637C1E7432C1C860891C6AC5F23D375B000247FB83E36C7D8BB8929F2C6A87A96D3812CB1641459EEAA1272EEA6D57375708B8766583F50ED31012A5";
    const uint8_t R[] = "00B36F1ED6798E7D6B81ED5D84EDE12F143E05DA90BBB9DC172EDE4693D5F639D78E2F144EBF22DCE6EFA133D42C7EF4DFD862521F6B885352678E5FD60962DCC6E7";
    const uint8_t S[] = "007DC54288D4F979BEC21057B333E98FF7B2912915706E553CF876D10F388965A5118EFAFC08F6CD1054549EE63CA0A987C89276DF6F72C64B0AB3D6ACAB0C559333";
#endif
    HASH_HandleTypeDef pParas;
    uint8_t *pQx = (uint8_t *)Q;
    uint8_t *pQy = (uint8_t *)((uint32_t)Q + lentmp);
    uint8_t *pR = (uint8_t *)r;
    uint8_t *pS = (uint8_t *)s;
    uint8_t temp;

    str2hex((const uint8_t *)Qx, pQx, lentmp);
#ifndef ECC_CRYPT_ITEMS_IN_BYTES
    for (cnt = 0; cnt < (lentmp >> 1); cnt++) {
        temp = pQx[cnt];
        pQx[cnt] = pQx[lentmp - 1 - cnt];
        pQx[lentmp - 1 - cnt] = temp;
    }
#endif
    str2hex((const uint8_t *)Qy, pQy, lentmp);
#ifndef ECC_CRYPT_ITEMS_IN_BYTES
    for (cnt = 0; cnt < (lentmp >> 1); cnt++) {
        temp = pQy[cnt];
        pQy[cnt] = pQy[lentmp - 1 - cnt];
        pQy[lentmp - 1 - cnt] = temp;
    }
#endif
    str2hex((const uint8_t *)R, pR, lentmp);
#ifndef ECC_CRYPT_ITEMS_IN_BYTES
    for (cnt = 0; cnt < (lentmp >> 1); cnt++) {
        temp = pR[cnt];
        pR[cnt] = pR[lentmp - 1 - cnt];
        pR[lentmp - 1 - cnt] = temp;
    }
#endif
    str2hex((const uint8_t *)S, pS, lentmp);
#ifndef ECC_CRYPT_ITEMS_IN_BYTES
    for (cnt = 0; cnt < (lentmp >> 1); cnt++) {
        temp = pS[cnt];
        pS[cnt] = pS[lentmp - 1 - cnt];
        pS[lentmp - 1 - cnt] = temp;
    }
#endif
    WDT_FEED;
    printf("***********ECDSA NIST standard test start*********\n");
    hash_once(&pParas, message, sizeof(message) >> 1, &sha1_cfg, Hm, 20);
    uint32_t Hm_len = getHashOutSize(&sha1_cfg);
#ifdef ECC_CRYPT_ITEMS_IN_BYTES
    word2str(Hm, 20);
    printf("[log]Hm:\t");
    for (int i = 0; i < Curve_NIST_P521.NumBytes; i++) {
        printf("%02X", ((uint8_t *)Hm)[i]);
    }
    printf("\n");
    printf("[log]Qx:\t");
    for (int i = 0; i < Curve_NIST_P521.NumBytes; i++) {
        printf("%02X", ((uint8_t *)Q)[i]);
    }
    printf("\n");
    printf("[log]Qy:\t");
    for (int i = 0; i < Curve_NIST_P521.NumBytes; i++) {
        printf("%02X", ((uint8_t *)Q)[i + Curve_NIST_P521.NumBytes]);
    }
    printf("\n");
    printf("[log]r :\t");
    for (int i = 0; i < Curve_NIST_P521.NumBytes; i++) {
        printf("%02X", ((uint8_t *)r)[i]);
    }
    printf("\n");
    printf("[log]s :\t");
    for (int i = 0; i < Curve_NIST_P521.NumBytes; i++) {
        printf("%02X", ((uint8_t *)s)[i]);
    }
    printf("\n");
#else
    printf("[log]Hm:\t");
    for (int i = 0; i < Curve_NIST_P521.NumWords; i++) {
        printf("%08X ", Hm[i]);
    }
    printf("\n");
    printf("[log]Qx:\t");
    for (int i = 0; i < Curve_NIST_P521.NumWords; i++) {
        printf("%08X ", Q[i]);
    }
    printf("\n");
    printf("[log]Qy:\t");
    for (int i = 0; i < Curve_NIST_P521.NumWords; i++) {
        printf("%08X ", Q[i + Curve_NIST_P521.NumWords]);
    }
    printf("\n");
    printf("[log]r :\t");
    for (int i = 0; i < Curve_NIST_P521.NumWords; i++) {
        printf("%08X ", r[i]);
    }
    printf("\n");
    printf("[log]s :\t");
    for (int i = 0; i < Curve_NIST_P521.NumWords; i++) {
        printf("%08X ", s[i]);
    }
    printf("\n");
#endif

    LL_ECC_sign_ver(&Curve_NIST_P521, (ECCdata_t *)Hm, Hm_len, (ECCdata_t *)Q, (ECCdata_t *)r, (ECCdata_t *)s, &ver_flag);
    printf("ECDSA Verify Result Output by NIST-P521\n");
    printf("\tVerify:\t:");
    if (ver_flag != PASS) {
        printf("FAIL\n");
        return;
    }
    printf("PASS\n");
    printf("***********ECDSA NIST standard test end*********\n");
}

/**
 * @brief ECDH verify by NIST standard data
 */
void ECDH_NIST256_test(void) {
    uint32_t key[8] = {0};
    uint32_t Q[16] = {0};
    uint32_t DH[16] = {0};
    uint32_t DHkey[16] = {0};

    int cnt;
    uint8_t *pkey = (uint8_t *)key;
    uint8_t *pQx = (uint8_t *)Q;
    uint8_t *pQy = (uint8_t *)Q + Curve_NIST_P256.NumBytes;
    uint8_t *pDHx = (uint8_t *)DH;
    uint8_t *pDHy = (uint8_t *)DH + Curve_NIST_P256.NumBytes;
    uint8_t temp;

    const uint8_t Key[] = "710735c8388f48c684a97bd66751cc5f5a122d6b9a96a2dbe73662f78217446d";
    const uint8_t Qx[] = "d0720dc691aa80096ba32fed1cb97c2b620690d06de0317b8618d5ce65eb728f";
    const uint8_t Qy[] = "9681b517b1cda17d0d83d335d9c4a8a9a9b0b1b3c7106d8f3c72bc5093dc275f";
    const uint8_t DHx[] = "1DB809C276F21610791168528EFA0185112E78655036AEED87C715A29045FDFC";
    const uint8_t DHy[] = "B0408E8868DE33C656A0761624B7EBA76169795B6BB156ABC7E1B2E2ECAB6B19";

    /*************String to Value*************/
    str2hex((const uint8_t *)Key, pkey, Curve_NIST_P256.NumBytes);
#ifndef ECC_CRYPT_ITEMS_IN_BYTES
    for (cnt = 0; cnt < (Curve_NIST_P256.NumBytes >> 1); cnt++) {
        temp = pkey[cnt];
        pkey[cnt] = pkey[Curve_NIST_P256.NumBytes - 1 - cnt];
        pkey[Curve_NIST_P256.NumBytes - 1 - cnt] = temp;
    }
#endif
    str2hex((const uint8_t *)Qx, pQx, Curve_NIST_P256.NumBytes);
#ifndef ECC_CRYPT_ITEMS_IN_BYTES
    for (cnt = 0; cnt < (Curve_NIST_P256.NumBytes >> 1); cnt++) {
        temp = pQx[cnt];
        pQx[cnt] = pQx[Curve_NIST_P256.NumBytes - 1 - cnt];
        pQx[Curve_NIST_P256.NumBytes - 1 - cnt] = temp;
    }
#endif
    str2hex((const uint8_t *)Qy, pQy, Curve_NIST_P256.NumBytes);
#ifndef ECC_CRYPT_ITEMS_IN_BYTES
    for (cnt = 0; cnt < (Curve_NIST_P256.NumBytes >> 1); cnt++) {
        temp = pQy[cnt];
        pQy[cnt] = pQy[Curve_NIST_P256.NumBytes - 1 - cnt];
        pQy[Curve_NIST_P256.NumBytes - 1 - cnt] = temp;
    }
#endif
    str2hex((const uint8_t *)DHx, pDHx, Curve_NIST_P256.NumBytes);
#ifndef ECC_CRYPT_ITEMS_IN_BYTES
    for (cnt = 0; cnt < (Curve_NIST_P256.NumBytes >> 1); cnt++) {
        temp = pDHx[cnt];
        pDHx[cnt] = pDHx[Curve_NIST_P256.NumBytes - 1 - cnt];
        pDHx[Curve_NIST_P256.NumBytes - 1 - cnt] = temp;
    }
#endif
    str2hex((const uint8_t *)DHy, pDHy, Curve_NIST_P256.NumBytes);
#ifndef ECC_CRYPT_ITEMS_IN_BYTES
    for (cnt = 0; cnt < (Curve_NIST_P256.NumBytes >> 1); cnt++) {
        temp = pDHy[cnt];
        pDHy[cnt] = pDHy[Curve_NIST_P256.NumBytes - 1 - cnt];
        pDHy[Curve_NIST_P256.NumBytes - 1 - cnt] = temp;
    }
#endif

    printf("***********ECDH NISTP256 test start*********\n");
    /*************1.ECDH generate shared secret*************/

    LL_ECC_DHkeygen(&Curve_NIST_P256, (ECCdata_t *)Q, (ECCdata_t *)key, (ECCdata_t *)DHkey);

    uint32_t _x[8] = {0};
    LL_ECC_DHkeygen_x(&Curve_NIST_P256, (ECCdata_t *)Q, (ECCdata_t *)key, (ECCdata_t *)_x);

    /*************2.ECDH verify and result output*************/
    printf("ECDH Verify Result Output by NIST-P256\n");
    printf("\tVerify:\t:");
    if ((0 != memcmp((const uint8_t *)DHkey, (const uint8_t *)DH, Curve_NIST_P256.NumBytes)) ||
        (0 != memcmp((const uint8_t *)_x, (const uint8_t *)DH, Curve_NIST_P256.NumBytes))) {
        printf("FAIL\n");
        return;
    }
    printf("PASS\n");
    printf("***********ECDH NISTP256 test end*********\n");
}

/**
 * @brief ECDH verify by NIST standard data
 */
void ECDH_NIST384_test(void) {
    uint32_t key[12] = {0};
    uint32_t Q[24] = {0};
    uint32_t DH[24] = {0};
    uint32_t DHkey[24] = {0};

    int cnt;
    uint8_t *pkey = (uint8_t *)key;
    uint8_t *pQx = (uint8_t *)Q;
    uint8_t *pQy = (uint8_t *)Q + Curve_NIST_P384.NumBytes;
    uint8_t *pDHx = (uint8_t *)DH;
    uint8_t *pDHy = (uint8_t *)DH + Curve_NIST_P384.NumBytes;
    uint8_t temp;

    const uint8_t Key[] = "5394f7973ea868c52bf3ff8d8ceeb4db90a683653b12485d5f627c3ce5abd8978fc9673d14a71d925747931662493c37";
    const uint8_t Qx[] = "732b0f83d303475584d88ed91cc74b367e9ffbfcc2d044d1485417d2731fa4f3b70347388e2308e9e43bdbf952465393";
    const uint8_t Qy[] = "d8d232a2c995a6ff133893dcfa9b559c11376eb999abf55edd51cc5edb7935500f80f55ca1a542a1b87f6c8c643b83d6";
    const uint8_t DHx[] = "F45136DF4DAFDDEEC2A03149A4016B7E540673665767E2CEAF4D171A3CE3A10032D30A47D15CE049A19A6808883F9D6D";
    const uint8_t DHy[] = "12A345A04F5847858C26E33CE097C073AA4A9647642D37AC12FD1F60543CF6A9E3CE05F4A88F4D26D83BF75494A54F1D";

    str2hex((const uint8_t *)Key, pkey, Curve_NIST_P384.NumBytes);
#ifndef ECC_CRYPT_ITEMS_IN_BYTES
    for (cnt = 0; cnt < (Curve_NIST_P384.NumBytes >> 1); cnt++) {
        temp = pkey[cnt];
        pkey[cnt] = pkey[Curve_NIST_P384.NumBytes - 1 - cnt];
        pkey[Curve_NIST_P384.NumBytes - 1 - cnt] = temp;
    }
#endif
    str2hex((const uint8_t *)Qx, pQx, Curve_NIST_P384.NumBytes);
#ifndef ECC_CRYPT_ITEMS_IN_BYTES
    for (cnt = 0; cnt < (Curve_NIST_P384.NumBytes >> 1); cnt++) {
        temp = pQx[cnt];
        pQx[cnt] = pQx[Curve_NIST_P384.NumBytes - 1 - cnt];
        pQx[Curve_NIST_P384.NumBytes - 1 - cnt] = temp;
    }
#endif
    str2hex((const uint8_t *)Qy, pQy, Curve_NIST_P384.NumBytes);
#ifndef ECC_CRYPT_ITEMS_IN_BYTES
    for (cnt = 0; cnt < (Curve_NIST_P384.NumBytes >> 1); cnt++) {
        temp = pQy[cnt];
        pQy[cnt] = pQy[Curve_NIST_P384.NumBytes - 1 - cnt];
        pQy[Curve_NIST_P384.NumBytes - 1 - cnt] = temp;
    }
#endif
    str2hex((const uint8_t *)DHx, pDHx, Curve_NIST_P384.NumBytes);
#ifndef ECC_CRYPT_ITEMS_IN_BYTES
    for (cnt = 0; cnt < (Curve_NIST_P384.NumBytes >> 1); cnt++) {
        temp = pDHx[cnt];
        pDHx[cnt] = pDHx[Curve_NIST_P384.NumBytes - 1 - cnt];
        pDHx[Curve_NIST_P384.NumBytes - 1 - cnt] = temp;
    }
#endif
    str2hex((const uint8_t *)DHy, pDHy, Curve_NIST_P384.NumBytes);
#ifndef ECC_CRYPT_ITEMS_IN_BYTES
    for (cnt = 0; cnt < (Curve_NIST_P384.NumBytes >> 1); cnt++) {
        temp = pDHy[cnt];
        pDHy[cnt] = pDHy[Curve_NIST_P384.NumBytes - 1 - cnt];
        pDHy[Curve_NIST_P384.NumBytes - 1 - cnt] = temp;
    }
#endif

    printf("***********ECDH NISTP384 test start*********\n");
    /*************1.ECDH generate shared secret*************/

    LL_ECC_DHkeygen(&Curve_NIST_P384, (ECCdata_t *)Q, (ECCdata_t *)key, (ECCdata_t *)DHkey);

    uint32_t _x[12] = {0};
    LL_ECC_DHkeygen_x(&Curve_NIST_P384, (ECCdata_t *)Q, (ECCdata_t *)key, (ECCdata_t *)_x);

    /*************2.ECDH verify and result output*************/
    printf("ECDH Verify Result Output by NIST-P384\n");
    printf("\tVerify:\t:");
    if ((0 != memcmp((const uint8_t *)DHkey, (const uint8_t *)DH, Curve_NIST_P384.NumBytes)) ||
        (0 != memcmp((const uint8_t *)_x, (const uint8_t *)DH, Curve_NIST_P384.NumBytes))) {
        printf("FAIL\n");
        return;
    }
    printf("PASS\n");
    printf("***********ECDH NISTP384 test end*********\n");
}

void ECC_keygen_test(void) {
    uint32_t pkey[12] = {0};
    uint32_t Q_1[24] = {0};
    uint32_t Q_2[24] = {0};
    uint32_t Q_3[25] = {0};
    uint32_t tmp_err = SEA_SUCCESS;

    printf("***********ECC sign and verify test start*********\n");
    tmp_err = LL_ECC_keygen(&Curve_NIST_P384, (ECCdata_t *)Q_1, (ECCdata_t *)pkey);
    TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);

    tmp_err = LL_ECC_public_keygen(&Curve_NIST_P384, (ECCdata_t *)Q_2, (ECCdata_t *)pkey);
    TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);

    printf("verify(LL_ECC_keygen & LL_ECC_public_keygen):\t");
    if (0 != memcmp((const uint8_t *)Q_2, (const uint8_t *)Q_1, Curve_NIST_P384.NumBytes)) {
        printf("FAIL\n");
        return;
    }
    printf("PASS\n");

#ifdef ECC_CRYPT_ITEMS_IN_BYTES
    tmp_err = LL_ECC_public_keygen_uncompressed(&Curve_NIST_P384, (ECCdata_t *)Q_3, (ECCdata_t *)pkey);
    TEST_ASSERT_EQUAL_UINT(SEA_SUCCESS, tmp_err);

    printf("verify(LL_ECC_keygen & LL_ECC_public_keygen_uncompressed):\t");
    if (0 != memcmp((const uint8_t *)((uint32_t)Q_3 + 1), (const uint8_t *)Q_1, Curve_NIST_P384.NumBytes)) {
        printf("FAIL\n");

        printf("Q_1:\t");
        for (int i = 0; i < Curve_NIST_P384.NumBytes; i++) {
            printf("%02X", *(uint8_t *)((uint32_t)Q_1 + i));
        }
        printf("\n");
        printf("Q_3:\t");
        for (int i = 0; i < (Curve_NIST_P384.NumBytes + 1); i++) {
            printf("%02X", *(uint8_t *)((uint32_t)Q_3 + i));
        }
        printf("\n");
        return;
    }
    printf("PASS\n");
#endif
    printf("***********ECC sign and verify test end*********\n");
}
