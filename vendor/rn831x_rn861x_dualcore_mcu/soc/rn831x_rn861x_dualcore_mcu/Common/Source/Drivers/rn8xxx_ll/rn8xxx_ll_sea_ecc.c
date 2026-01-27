/******************************************************************************
 * @file        rn8xxx_ll_sea_ecc.c
 * @brief       ECC functions
 * @author      Renergy Technology
 *
 * @note
 * Copyright (C) , Renergy Technology Inc. All rights reserved.
 *****************************************************************************/
#include "rn8xxx_ll_sea_common.h"
#include "rn8xxx_ll_sea_ecc.h"
#include "rn8xxx_ll_sea_trng.h"

#ifndef  RN821x_RN721x_SOC_V3

#if !defined(RN831x_RN861x_MCU_V4)

#define ECC_PUBKEY_HEADER_BYTE 0x04

static void  SEA_ECC_IR_FUNC_RUN(uint8_t IR_FUNC, uint32_t TIMEOUT_CFG, ErrStatus *ERR_RET);
static ErrStatus ecc_cal_J0(const uint32_t J0[]);
ErrStatus ecc_cal_H(uint16_t num,const uint32_t P[]);
static ErrStatus ecc_dbl_i2m(uint8_t wnum, const uint32_t A[], const uint32_t Q0_X[], const uint32_t Q0_Y[]);
static void ecc_copy2Q1(uint8_t wnum);
static uint8_t ecc_numdigits(const uint32_t vli[], uint8_t max_words);
static ErrStatus ecc_point_double(void);
static ErrStatus ecc_m2i(void);
static int ecc_cmp_unsafe(const uint32_t left[], const uint32_t right[], uint8_t wnum);
static ErrStatus ecc_modadd(const uint32_t P[], const uint32_t X1[], const uint32_t X2[], uint8_t wnum, uint16_t bnum, uint32_t result[]);
#if 0
static ErrStatus ecc_modsub(const uint32_t P[],const uint32_t X1[],const uint32_t X2[], uint8_t wnum, uint16_t bnum,uint32_t result[]);
#endif
static ErrStatus ecc_point_add(void);

static ErrStatus ecc_point_mult(const uint32_t  P[],const uint32_t A[], const uint32_t  pkey[], const uint32_t X[], const uint32_t Y[], uint8_t wnum, uint16_t bnum
                     ,uint32_t resultX[], uint32_t resultY[]);
static ErrStatus ecc_modmult(const uint32_t P[], const uint32_t X1[], const uint32_t X2[], uint8_t wnum, uint16_t bnum, uint32_t result[]);
static ErrStatus ecc_point_oncurve(const uint32_t P[], const uint32_t A[], const uint32_t B[], const uint32_t X[], const uint32_t Y[],  uint8_t wnum,  uint16_t bnum, PassFlag* ver_flag);
static ErrStatus ecc_modinv(const uint32_t P[], const uint32_t U[], uint8_t wnum, uint16_t bnum, uint32_t result[], PassFlag *flag);
static ErrStatus ecc_add_i2m(uint8_t wnum, const uint32_t A[], const uint32_t Q0_X[], const uint32_t Q0_Y[], const uint32_t Q1_X[], const uint32_t Q1_Y[]);
static void ecc_infinity(const uint8_t wnum,PassFlag* infinity_status);

#ifdef ECC_CRYPT_ITEMS_IN_BYTES
static void str2word(const uint8_t *str, uint32_t *words, uint32_t bytelen);
#endif
#endif

/******************************************************************************
 * Configuration of the Elliptic curve type
 * Allowed values:
 *     - NIST_P192 for a 192 bit ECC curve support NIST Standards
 *     - NIST_P224 for a 224 bit ECC curve support NIST Standards
 *     - NIST_P256 for a 256 bit ECC curve support NIST Standards
 *     - NIST_P384 for a 384 bit ECC curve support NIST Standards
 *     - NIST_P521 for a 521 bit ECC curve support NIST Standards
 *     - Brainpool_P256 for a 256 bit ECC curve support Brainpoolr1 Standards
 *     - Brainpool_P384 for a 384 bit ECC curve support Brainpoolr1 Standards
 *     - Brainpool_P512 for a 512 bit ECC curve support Brainpoolr1 Standards
 *****************************************************************************/
const uECC_Curve_t Curve_NIST_P192 = {
    6                                                                   /*num_words*/
  , 24                                                                  /*num_bytes*/
  , 192                                                                 /*num_bits */
  , {0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFEU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,
     0U         ,0U         ,0U         ,0U         ,0U         ,0U         ,
     0U         ,0U         ,0U         ,0U         ,0U         }            /*p*/
  , {0xB4D22831U,0x146BC9B1U,0x99DEF836U,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,
     0U         ,0U         ,0U         ,0U         ,0U         ,0U         ,
     0U         ,0U         ,0U         ,0U         ,0U         }            /*n*/
  , {0xFFFFFFFCU,0xFFFFFFFFU,0xFFFFFFFEU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,
     0U         ,0U         ,0U         ,0U         ,0U         ,0U         ,
     0U         ,0U         ,0U         ,0U         ,0U         }            /*a*/
  , {0xC146B9B1U,0xFEB8DEECU,0x72243049U,0x0FA7E9ABU,0xE59C80E7U,0x64210519U,
     0U         ,0U         ,0U         ,0U         ,0U         ,0U         ,
     0U         ,0U         ,0U         ,0U         ,0U         }            /*b*/
  , {0x82FF1012U,0xF4FF0AFDU,0x43A18800U,0x7CBF20EBU,0xB03090F6U,0x188DA80EU,
     0U         ,0U         ,0U         ,0U         ,0U         ,0U         ,
     0U         ,0U         ,0U         ,0U         ,0U                      /*Gx*/
  ,  0x1E794811U,0x73F977A1U,0x6B24CDD5U,0x631011EDU,0xFFC8DA78U,0x07192B95U,
     0U         ,0U         ,0U         ,0U         ,0U         ,0U         ,
     0U         ,0U         ,0U         ,0U         ,0U         }            /*Gy*/
};

const uECC_Curve_t Curve_NIST_P224 = {
    7                                                                   /*num_words*/
  , 28                                                                  /*num_bytes*/
  , 224                                                                 /*num_bits*/
  , {0x00000001U,0x00000000U,0x00000000U,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,
     0xFFFFFFFFU,0U         ,0U         ,0U         ,0U         ,0U         ,
     0U         ,0U         ,0U         ,0U         ,0U         }            /*p*/
  , {0x5C5C2A3DU,0x13DD2945U,0xE0B8F03EU,0xFFFF16A2U,0xFFFFFFFFU,0xFFFFFFFFU,
     0xFFFFFFFFU,0U         ,0U         ,0U         ,0U         ,0U         ,
     0U         ,0U         ,0U         ,0U         ,0U         }            /*n*/
  , {0xFFFFFFFEU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFEU,0xFFFFFFFFU,0xFFFFFFFFU,
     0xFFFFFFFFU,0U         ,0U         ,0U         ,0U         ,0U         ,
     0U         ,0U         ,0U         ,0U         ,0U         }            /*a*/
  , {0x2355FFB4U,0x270B3943U,0xD7BFD8BAU,0x5044B0B7U,0xF5413256U,0x0C04B3ABU,
     0xB4050A85U,0U         ,0U         ,0U         ,0U         ,0U         ,
     0U         ,0U         ,0U         ,0U         ,0U         }            /*b*/
  , {0x115C1D21U,0x343280D6U,0x56C21122U,0x4A03C1D3U,0x321390B9U,0x6BB4BF7FU,
     0xB70E0CBDU,0U         ,0U         ,0U         ,0U         ,0U         ,
     0U         ,0U         ,0U         ,0U         ,0U                      /*Gx*/
  ,  0x85007E34U,0x44D58199U,0x5A074764U,0xCD4375A0U,0x4C22DFE6U,0xB5F723FBU,
     0xBD376388U,0U         ,0U         ,0U         ,0U         ,0U         ,
     0U         ,0U         ,0U         ,0U         ,0U         }            /*Gy*/
};


const uECC_Curve_t Curve_NIST_P256 = {
    8                                                                   /*num_words*/
  , 32                                                                  /*num_bytes*/
  , 256                                                                 /*num_bits */
  , {0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0x00000000U,0x00000000U,0x00000000U,
     0x00000001U,0xFFFFFFFFU,0U         ,0U         ,0U         ,0U         ,
     0U         ,0U         ,0U         ,0U         ,0U         }            /*p*/
  , {0xFC632551U,0xF3B9CAC2U,0xA7179E84U,0xBCE6FAADU,0xFFFFFFFFU,0xFFFFFFFFU,
     0x00000000U,0xFFFFFFFFU,0U         ,0U         ,0U         ,0U         ,
     0U         ,0U         ,0U         ,0U         ,0U         }            /*n*/
  , {0xFFFFFFFCU,0xFFFFFFFFU,0xFFFFFFFFU,0x00000000U,0x00000000U,0x00000000U,
     0x00000001U,0xFFFFFFFFU,0U         ,0U         ,0U         ,0U         ,
     0U         ,0U         ,0U         ,0U         ,0U         }            /*a*/
  , {0x27D2604BU,0x3BCE3C3EU,0xCC53B0F6U,0x651D06B0U,0x769886BCU,0xB3EBBD55U,
     0xAA3A93E7U,0x5AC635D8U,0U         ,0U         ,0U         ,0U         ,
     0U         ,0U         ,0U         ,0U         ,0U         }            /*b*/
  , {0xD898C296U,0xF4A13945U,0x2DEB33A0U,0x77037D81U,0x63A440F2U,0xF8BCE6E5U,
     0xE12C4247U,0x6B17D1F2U,0U         ,0U         ,0U         ,0U         ,
     0U         ,0U         ,0U         ,0U         ,0U                      /*Gx*/
  ,  0x37BF51F5U,0xCBB64068U,0x6B315ECEU,0x2BCE3357U,0x7C0F9E16U,0x8EE7EB4AU,
     0xFE1A7F9BU,0x4FE342E2U,0U         ,0U         ,0U         ,0U         ,
     0U         ,0U         ,0U         ,0U         ,0U         }            /*Gy*/
};

const uECC_Curve_t Curve_NIST_P384 = {
    12                                                                  /*num_words*/
  , 48                                                                  /*num_bytes*/
  , 384                                                                 /*num_bits */
  , {0xFFFFFFFFU,0x00000000U,0x00000000U,0xFFFFFFFFU,0xFFFFFFFEU,0xFFFFFFFFU,
     0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,
     0U         ,0U         ,0U         ,0U         ,0U         }            /*p*/
  , {0xCCC52973U,0xECEC196AU,0x48B0A77AU,0x581A0DB2U,0xF4372DDFU,0xC7634D81U,
     0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,
     0U         ,0U         ,0U         ,0U         ,0U         }            /*n*/
  , {0xFFFFFFFCU,0x00000000U,0x00000000U,0xFFFFFFFFU,0xFFFFFFFEU,0xFFFFFFFFU,
     0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,
     0U         ,0U         ,0U         ,0U         ,0U         }            /*a*/
  , {0xD3EC2AEFU,0x2A85C8EDU,0x8A2ED19DU,0xC656398DU,0x5013875AU,0x0314088FU,
     0xFE814112U,0x181D9C6EU,0xE3F82D19U,0x988E056BU,0xE23EE7E4U,0xB3312FA7U,
     0U         ,0U         ,0U         ,0U         ,0U         }            /*b*/
  , {0x72760AB7U,0x3A545E38U,0xBF55296CU,0x5502F25DU,0x82542A38U,0x59F741E0U,
     0x8BA79B98U,0x6E1D3B62U,0xF320AD74U,0x8EB1C71EU,0xBE8B0537U,0xAA87CA22U,
     0U         ,0U         ,0U         ,0U         ,0U                      /*Gx*/
  ,  0x90EA0E5FU,0x7A431D7CU,0x1D7E819DU,0x0A60B1CEU,0xB5F0B8C0U,0xE9DA3113U,
     0x289A147CU,0xF8F41DBDU,0x9292DC29U,0x5D9E98BFU,0x96262C6FU,0x3617DE4AU,
     0U         ,0U         ,0U         ,0U         ,0U         }            /*Gy*/
};


const uECC_Curve_t Curve_NIST_P521 = {
    17                                                                  /*num_words*/
  , 66                                                                  /*num_bytes*/
  , 521                                                                 /*num_bits */
  , {0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,
     0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,
     0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0x000001FFU}            /*p*/
  , {0x91386409U,0xBB6FB71EU,0x899C47AEU,0x3BB5C9B8U,0xF709A5D0U,0x7FCC0148U,
     0xBF2F966BU,0x51868783U,0xFFFFFFFAU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,
     0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0x000001FFU}            /*n*/
  , {0xFFFFFFFCU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,
     0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,
     0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0x000001FFU}            /*a*/
  , {0x6B503F00U,0xEF451FD4U,0x3D2C34F1U,0x3573DF88U,0x3BB1BF07U,0x1652C0BDU,
     0xEC7E937BU,0x56193951U,0x8EF109E1U,0xB8B48991U,0x99B315F3U,0xA2DA725BU,
     0xB68540EEU,0x929A21A0U,0x8E1C9A1FU,0x953EB961U,0x00000051U}            /*b*/
  , {0xC2E5BD66U,0xF97E7E31U,0x856A429BU,0x3348B3C1U,0xA2FFA8DEU,0xFE1DC127U,
     0xEFE75928U,0xA14B5E77U,0x6B4D3DBAU,0xF828AF60U,0x053FB521U,0x9C648139U,
     0x2395B442U,0x9E3ECB66U,0x0404E9CDU,0x858E06B7U,0x000000C6U             /*Gx*/
  ,  0x9FD16650U,0x88BE9476U,0xA272C240U,0x353C7086U,0x3FAD0761U,0xC550B901U,
     0x5EF42640U,0x97EE7299U,0x273E662CU,0x17AFBD17U,0x579B4468U,0x98F54449U,
     0x2C7D1BD9U,0x5C8A5FB4U,0x9A3BC004U,0x39296A78U,0x00000118U}            /*Gy*/
};

const uECC_Curve_t Curve_Brainpool_256 = {
    8                                                                   /*num_words*/
  , 32                                                                  /*num_bytes*/
  , 256                                                                 /*num_bits */
  , {0x1F6E5377U,0x2013481DU,0xD5262028U,0x6E3BF623U,0x9D838D72U,0x3E660A90U,
     0xA1EEA9BCU,0xA9FB57DBU,0U         ,0U         ,0U         ,0U         ,
     0U         ,0U         ,0U         ,0U         ,0U         }            /*p*/
  , {0x974856A7U,0x901E0E82U,0xB561A6F7U,0x8C397AA3U,0x9D838D71U,0x3E660A90U,
     0xA1EEA9BCU,0xA9FB57DBU,0U         ,0U         ,0U         ,0U         ,
     0U         ,0U         ,0U         ,0U         ,0U         }            /*n*/
  , {0xF330B5D9U,0xE94A4B44U,0x26DC5C6CU,0xFB8055C1U,0x417AFFE7U,0xEEF67530U,
     0xFC2C3057U,0x7D5A0975U,0U         ,0U         ,0U         ,0U         ,
     0U         ,0U         ,0U         ,0U         ,0U         }            /*a*/
  , {0xFF8C07B6U,0x6BCCDC18U,0x5CF7E1CEU,0x95841629U,0xBBD77CBFU,0xF330B5D9U,
     0xE94A4B44U,0x26DC5C6CU,0U         ,0U         ,0U         ,0U         ,
     0U         ,0U         ,0U         ,0U         ,0U         }            /*b*/
  , {0x9ACE3262U,0x3A4453BDU,0xE3BD23C2U,0xB9DE27E1U,0xFC81B7AFU,0x2C4B482FU,
     0xCB7E57CBU,0x8BD2AEB9U,0U         ,0U         ,0U         ,0U         ,
     0U         ,0U         ,0U         ,0U         ,0U                      /*Gx*/
  ,  0x2F046997U,0x5C1D54C7U,0x2DED8E54U,0xC2774513U,0x14611DC9U,0x97F8461AU,
     0xC3DAC4FDU,0x547EF835U,0U         ,0U         ,0U         ,0U         ,
     0U         ,0U         ,0U         ,0U         ,0U         }            /*Gy*/
};

const uECC_Curve_t Curve_Brainpool_384 = {
    12                                                                  /*num_words*/
  , 48                                                                  /*num_bytes*/
  , 384                                                                 /*num_bits */
  , {0x3107EC53U,0x87470013U,0x901D1A71U,0xACD3A729U,0x7FB71123U,0x12B1DA19U,
     0xED5456B4U,0x152F7109U,0x50E641DFU,0x0F5D6F7EU,0xA3386D28U,0x8CB91E82U,
     0U         ,0U         ,0U         ,0U         ,0U         }            /*p*/
  , {0xE9046565U,0x3B883202U,0x6B7FC310U,0xCF3AB6AFU,0xAC0425A7U,0x1F166E6CU,
     0xED5456B3U,0x152F7109U,0x50E641DFU,0x0F5D6F7EU,0xA3386D28U,0x8CB91E82U,
     0U         ,0U         ,0U         ,0U         ,0U         }            /*n*/
  , {0x22CE2826U,0x04A8C7DDU,0x503AD4EBU,0x8AA5814AU,0xBA91F90FU,0x139165EFU,
     0x4FB22787U,0xC2BEA28EU,0xCE05AFA0U,0x3C72080AU,0x3D8C150CU,0x7BC382C6U,
     0U         ,0U         ,0U         ,0U         ,0U         }            /*a*/
  , {0xFA504C11U,0x3AB78696U,0x95DBC994U,0x7CB43902U,0x3EEB62D5U,0x2E880EA5U,
     0x07DCD2A6U,0x2FB77DE1U,0x16F0447CU,0x8B39B554U,0x22CE2826U,0x04A8C7DDU,
     0U         ,0U         ,0U         ,0U         ,0U         }            /*b*/
  , {0x47D4AF1EU,0xEF87B2E2U,0x36D646AAU,0xE826E034U,0x0CBD10E8U,0xDB7FCAFEU,
     0x7EF14FE3U,0x8847A3E7U,0xB7C13F6BU,0xA2A63A81U,0x68CF45FFU,0x1D1C64F0U,
     0U         ,0U         ,0U         ,0U         ,0U                      /*Gx*/
  ,  0x263C5315U,0x42820341U,0x77918111U,0x0E464621U,0xF9912928U,0xE19C054FU,
     0xFEEC5864U,0x62B70B29U,0x95CFD552U,0x5CB1EB8EU,0x20F9C2A4U,0x8ABE1D75U,
     0U         ,0U         ,0U         ,0U         ,0U         }            /*Gy*/
};

const uECC_Curve_t Curve_Brainpool_512 = {
    16                                                                  /*num_words*/
  , 64                                                                  /*num_bytes*/
  , 512                                                                 /*num_bits */
  , {0x583A48F3U,0x28AA6056U,0x2D82C685U,0x2881FF2FU,0xE6A380E6U,0xAECDA12AU,
     0x9BC66842U,0x7D4D9B00U,0x70330871U,0xD6639CCAU,0xB3C9D20EU,0xCB308DB3U,
     0x33C9FC07U,0x3FD4E6AEU,0xDBE9C48BU,0xAADD9DB8U,0U         }            /*p*/
  , {0x9CA90069U,0xB5879682U,0x085DDADDU,0x1DB1D381U,0x7FAC1047U,0x41866119U,
     0x4CA92619U,0x553E5C41U,0x70330870U,0xD6639CCAU,0xB3C9D20EU,0xCB308DB3U,
     0x33C9FC07U,0x3FD4E6AEU,0xDBE9C48BU,0xAADD9DB8U,0U         }            /*n*/
  , {0x77FC94CAU,0xE7C1AC4DU,0x2BF2C7B9U,0x7F1117A7U,0x8B9AC8B5U,0x0A2EF1C9U,
     0xA8253AA1U,0x2DED5D5AU,0xEA9863BCU,0xA83441CAU,0x3DF91610U,0x94CBDD8DU,
     0xAC234CC5U,0xE2327145U,0x8B603B89U,0x7830A331U,0U         }            /*a*/
  , {0x8016F723U,0x2809BD63U,0x5EBAE5DDU,0x984050B7U,0xDC083E67U,0x77FC94CAU,
     0xE7C1AC4DU,0x2BF2C7B9U,0x7F1117A7U,0x8B9AC8B5U,0x0A2EF1C9U,0xA8253AA1U,
     0x2DED5D5AU,0xEA9863BCU,0xA83441CAU,0x3DF91610U,0U         }            /*b*/
  , {0xBCB9F822U,0x8B352209U,0x406A5E68U,0x7C6D5047U,0x93B97D5FU,0x50D1687BU,
     0xE2D0D48DU,0xFF3B1F78U,0xF4D0098EU,0xB43B62EEU,0xB5D916C1U,0x85ED9F70U,
     0x9C4C6A93U,0x5A21322EU,0xD82ED964U,0x81AEE4BDU,0U                      /*Gx*/
  ,  0x3AD80892U,0x78CD1E0FU,0xA8F05406U,0xD1CA2B2FU,0x8A2763AEU,0x5BCA4BD8U,
     0x4A5F485EU,0xB2DCDE49U,0x881F8111U,0xA000C55BU,0x24A57B1AU,0xF209F700U,
     0xCF7822FDU,0xC0EABFA9U,0x566332ECU,0x7DDE385DU,0U         }            /*Gy*/
};

#if !defined(RN831x_RN861x_MCU_V4)
#if 0
#define  SEA_ECC_IR_FUNC_RUN(IR_FUNC, TIMEOUT_CFG, ERR_RET)     \
         do{\
         uint32_t cnt = 0U;\
         SEA_CYPHER->CTRL = (IR_FUNC);\
         SEA_CYPHER->Status = 1U;\
         SEA_CYPHER->START  = 1U;\
         while((SEA_CYPHER->Status & 1U)==0U){\
            if(cnt++ > (TIMEOUT_CFG)){\
                (ERR_RET) = ERROR;\
                setErrorCode(ECC_TIMEOUT_FAILURE) ;\
                break  ;\
            }\
         }\
     }while(0U)
#else
static void  SEA_ECC_IR_FUNC_RUN(uint8_t IR_FUNC, uint32_t TIMEOUT_CFG, ErrStatus *ERR_RET)
{
    uint32_t cnt = 0U;
    SEA_CYPHER->CTRL = (IR_FUNC);
    SEA_CYPHER->Status = 1U;
    SEA_CYPHER->START  = 1U;
    while((SEA_CYPHER->Status & 1U)==0U){
        if(cnt++ > (TIMEOUT_CFG)){
            *(ERR_RET) = ERROR;
            setErrorCode(ECC_TIMEOUT_FAILURE) ;
            break  ;
        }
    }
}
#endif

/**
 * @brief ecc_cmp_unsafe
 * @param left the left num
 * @param right the right num
 * @param wnum the expected comparing length
 *
 * @return 1:left>right; -1:left<right; 0:left=right
 */
static int ecc_cmp_unsafe(const uint32_t left[], const uint32_t right[], uint8_t wnum)
{
    uint8_t i = wnum -1U;
    int tmp_ret = 0;
    do
    {
      if(left[i] != right[i]){
        if(left[i] > right[i]){
          tmp_ret =   1;
        }
        else{
          tmp_ret =  -1;
        }
        break;
      }
    }while(0U != i--);
    return tmp_ret;
}

/**
 * @brief ecc_modadd
 * @param P modular value
 * @param X1 operand 1
 * @param X2 operand 2
 * @param wnum word length
 * @param bnum bit length
 *
 * @return result
 */
static ErrStatus ecc_modadd(const uint32_t P[], const uint32_t X1[], const uint32_t X2[], uint8_t wnum, uint16_t bnum, uint32_t result[])
{
    uint8_t i;
    ErrStatus tmp_err =  SUCCESS;

    if(wnum > 17U)
    {
        tmp_err = ERROR;
        setErrorCode(ECC_PARASLEN_FAILURE) ;
    }
    else
    {
        for( i = 0U; i < wnum; i++){
            SEA_ECC_MOD_P->REG[i]     = P[i];
            SEA_ECC_MOD_PT->REG[i]    = P[i];
            SEA_ECC_MODADD_ADDX->REG[i]  = X1[i];
            SEA_ECC_MODADD_ADDY->REG[i]  = X2[i];
        }
        /*64bit expand*/
        for(i = wnum; i < (wnum + 2U); i++){
            SEA_ECC_MODADD_ADDX->REG[i] = 0U;
            SEA_ECC_MODADD_ADDY->REG[i] = 0U;
        }

        SEA_CYPHER->NLen = bnum;

        SEA_ECC_IR_FUNC_RUN(SEA_CTRL_IR_ECC_ModAdd, SEA_TIMEOUT_IR_ECC_ModAdd, &tmp_err);

        for( i = 0U; i < wnum; i++){
            result[i] = SEA_ECC_MODADD_ADDX->REG[i];
        }
     }
    return tmp_err;
}
#if 0
/**
 * @brief ecc_modsub
 * @param P modular value
 * @param X1 operand 1
 * @param X2 operand 2
 * @param wnum word length
 * @param bnum bit length
 *
 * @return result
 */
ErrStatus ecc_modsub(const uint32_t P[],const uint32_t X1[],const uint32_t X2[], uint8_t wnum, uint16_t bnum,uint32_t result[])
{
    uint8_t i;
    ErrStatus tmp_err =  SUCCESS;
    if(wnum > 18U)
    {
        tmp_err = ERROR;
        setErrorCode(ECC_PARASLEN_FAILURE) ;
    }
    else
    {
        for( i = 0U; i < wnum; i++)
        {
            SEA_ECC_MOD_P->REG[i]     = P[i];
            SEA_ECC_MOD_PT->REG[i]    = P[i];
            SEA_ECC_MODADD_ADDX->REG[i]  = X1[i];
            SEA_ECC_MODADD_ADDY->REG[i]  = X2[i];
        }
        for(i = wnum; i < wnum +2U; i++)
        {
            SEA_ECC_MODADD_ADDX->REG[i] = 0U;
            SEA_ECC_MODADD_ADDY->REG[i] = 0U;
        }
        SEA_CYPHER->NLen = bnum;

        SEA_ECC_IR_FUNC_RUN(SEA_CTRL_IR_ECC_ModSub, SEA_TIMEOUT_IR_ECC_ModSub, &tmp_err);

        for( i = 0U; i < wnum; i++)
        {
            result[i] = SEA_ECC_MODADD_ADDX->REG[i];
        }
    }
    return tmp_err;
}
#endif
/**
 * @brief ecc_cal_J0
 * @param J0 modular value low 64 bits
 *
 */
static ErrStatus ecc_cal_J0(const uint32_t J0[])
{
    ErrStatus tmp_err =  SUCCESS;

    SEA_CYPHER->N0Low  = J0[0];
    SEA_CYPHER->N0High = J0[1];

    SEA_ECC_IR_FUNC_RUN(SEA_CTRL_IR_PKA_J0_CAL, SEA_TIMEOUT_IR_PKA_J0_CAL, &tmp_err);

    return tmp_err;
}
/**
 * @brief ecc_cal_H
 * @param num modular length
 * @param P modular value
 *
 */
ErrStatus ecc_cal_H(uint16_t num,const uint32_t P[])
{
    uint16_t temp;
    uint32_t i;
    ErrStatus tmp_err =  SUCCESS;

    for( i = 0U; i < ((uint32_t)num >> 5); i++)
    {
        SEA_ECC_MOD_P->REG[i] = P[i];
        SEA_ECC_MOD_PT->REG[i] = P[i];
        SEA_ECC_H->REG[i] = 0U;
    }
    temp = num & 0x1fU;

    SEA_ECC_H->REG[i] = (uint32_t)1U << (uint32_t)temp;

    if(0U != temp)
    {
        SEA_ECC_MOD_P->REG[i] = P[i];
        SEA_ECC_MOD_PT->REG[i] = P[i];
    }
    SEA_CYPHER->NLen = num;

    SEA_ECC_IR_FUNC_RUN(SEA_CTRL_IR_ECC_H_CAL, SEA_TIMEOUT_IR_ECC_H_CAL, &tmp_err);

    return tmp_err;
}
/**
 * @brief ecc_dbl_i2m
 * @param wnum word length
 * @param A curve parameter a
 * @param Q0_X vector Q0 x-coordinate
 * @param Q0_Y vector Q0 y-coordinate
 */
static ErrStatus ecc_dbl_i2m(uint8_t wnum,const uint32_t A[],const uint32_t Q0_X[], const uint32_t Q0_Y[])
{
    uint32_t i;
    ErrStatus tmp_err =  SUCCESS;

    SEA_ECC_MAP_CONST->REG[0] = 1U;
    SEA_ECC_MAP_Q0_Z->REG[0] = 1U;
    for(i = 1U; i < wnum; i++)
    {
        SEA_ECC_MAP_CONST->REG[i] = 0U;
        SEA_ECC_MAP_Q0_Z->REG[i] = 0U;
    }
    for(i = 0U; i < wnum; i++)
    {
        SEA_ECC_MAP_A->REG[i] = A[i];
        SEA_ECC_MAP_Q0_X->REG[i] = Q0_X[i];
        SEA_ECC_MAP_Q0_Y->REG[i] = Q0_Y[i];
    }

    SEA_ECC_IR_FUNC_RUN(SEA_CTRL_IR_ECC_DBL_I2M, SEA_TIMEOUT_IR_ECC_DBL_I2M, &tmp_err);

    return tmp_err;
}
/**
 * @brief ecc_copy2Q1
 * @param wnum word length
 */
static void ecc_copy2Q1(uint8_t wnum)
{
    uint8_t i;
    for(i = 0U; i < wnum; i++)
    {
        SEA_ECC_MAP_Q1_X->REG[i] = SEA_ECC_MAP_Q0_X->REG[i];
        SEA_ECC_MAP_Q1_Y->REG[i] = SEA_ECC_MAP_Q0_Y->REG[i];
        SEA_ECC_MAP_Q1_Z->REG[i] = SEA_ECC_MAP_Q0_Z->REG[i];
    }
}
/**
 * @brief ecc_numdigits
 * @param vli value
 * @param max_words value maximun word length
 *
 */
static uint8_t ecc_numdigits(const uint32_t vli[], uint8_t max_words)
{
    uint8_t i = max_words - 1U ;
    do{
      if(vli[i] != 0U ){
        break;
      }
    }while(0U != i--);

    return i+1U;
}
/**
 * @brief ecc_point_double
 *
 */
static ErrStatus ecc_point_double(void)
{
    ErrStatus tmp_err =  SUCCESS;

    SEA_ECC_IR_FUNC_RUN(SEA_CTRL_IR_ECC_Point_DBL, SEA_TIMEOUT_IR_ECC_Point_DBL, &tmp_err);

    return tmp_err;
}
/**
 * @brief ecc_point_add
 *
 */
static ErrStatus ecc_point_add(void)
{
    ErrStatus tmp_err =  SUCCESS;
    SEA_ECC_IR_FUNC_RUN(SEA_CTRL_IR_ECC_Point_ADD, SEA_TIMEOUT_IR_ECC_Point_ADD, &tmp_err);
    return tmp_err;
}
/**
 * @brief ecc_m2i
 *
 */
static ErrStatus ecc_m2i(void)
{
    ErrStatus tmp_err =  SUCCESS;

    SEA_ECC_IR_FUNC_RUN(SEA_CTRL_IR_ECC_M2I, SEA_TIMEOUT_IR_ECC_M2I, &tmp_err);

    return tmp_err;
}
/**
 * @brief ecc_point_mult
 * @param P modular value
 * @param A curve parameter a
 * @param pkey scalar
 * @param X vector x-coordinate
 * @param Y vector y-coordinate
 * @param wnum word length
 * @param bnum bit length
 *
 * @return resultX result vector x-coordinate
 * @return resultY result vector y-coordinate
 */
static ErrStatus ecc_point_mult(const uint32_t P[], const uint32_t A[], const uint32_t pkey[], const uint32_t X[], const uint32_t Y[], uint8_t wnum, uint16_t bnum, uint32_t resultX[], uint32_t resultY[])
{
    int len;
    int m;
    int i;
    uint32_t chk;
    uint32_t num_words;
    uint16_t num_bits;
    ErrStatus tmp_err =  SUCCESS;

    (void)ecc_cal_J0(P);
    (void)ecc_cal_H(bnum,P);
    (void)ecc_dbl_i2m(wnum,A,X,Y);
    ecc_copy2Q1(wnum);
    num_words = ecc_numdigits(pkey,wnum);

    chk = pkey[num_words - 1U];
    i = 0;
    while(0U != chk){
        i++;
        chk >>= 1U;
    }

    num_bits = (uint16_t)i;
    for(len = (int)num_words -1 ; len >= 0; len--)
    {
        if(len != ((int)num_words -1))
        {
            for(m = 31; m >= 0; m--)
            {
                if(SUCCESS != ecc_point_double()){
                  tmp_err = ERROR;
                }
                chk = pkey[len] & ((uint32_t)1U << (uint32_t)m);
                if(0U != chk){
                    (void)ecc_point_add();
                }
            }
        }
        else
        {
            for(m = (int)num_bits - 2; m >= 0; m--)
            {
                if(SUCCESS != ecc_point_double()){
                  tmp_err = ERROR;
                }
                chk = pkey[len] & ((uint32_t)1U << (uint32_t)m);
                if(0U != chk)
                {
                    (void)ecc_point_add();
                }
            }
        }
        if(SUCCESS != tmp_err){
          break;
        }
    }
    (void)ecc_m2i();
    for(i = 0; i < (int)wnum; i++)
    {
        resultX[i] = SEA_ECC_MAP_Q0_X->REG[i];
        resultY[i] = SEA_ECC_MAP_Q0_Y->REG[i];
    }

    return tmp_err;
}
/**
 * @brief ecc_modmult
 * @param P modular value
 * @param X1 the one
 * @param X2 the other 0ne
 * @param wnum word length
 * @param bnum bit length
 *
 * @return result x*y mod P
 *
 */
static ErrStatus ecc_modmult(const uint32_t P[], const uint32_t X1[], const uint32_t X2[], uint8_t wnum, uint16_t bnum, uint32_t result[])
{
    uint32_t i;
    ErrStatus tmp_err = SUCCESS;

    (void)ecc_cal_J0(P);
    (void)ecc_cal_H(bnum, P);

    for( i = 0U; i < wnum; i++)
    {
        SEA_ECC_MAP_Q0_X->REG[i] = X1[i];
        SEA_ECC_MAP_Q0_Y->REG[i] = X2[i];
    }

    SEA_ECC_IR_FUNC_RUN(SEA_CTRL_IR_ECC_ModMul, SEA_TIMEOUT_IR_ECC_ModMul, &tmp_err);

    for( i = 0U; i < wnum; i++)
    {
        result[i] = SEA_ECC_MAP_Q0_X->REG[i];
    }
    return  tmp_err;
}
/**
 * @brief ecc_point_oncurve
 * @param P modular value
 * @param A curve parameter a
 * @param B curve parameter b
 * @param X vector x-coordinate
 * @param Y vector y-coordinate
 * @param wnum word length
 * @param bnum bit length
 *
 * @return ver_flag: PASS-0, FALI-1
 */
static ErrStatus ecc_point_oncurve(const uint32_t P[], const uint32_t A[], const uint32_t B[], const uint32_t X[], const uint32_t Y[],  uint8_t wnum,  uint16_t bnum, PassFlag *ver_flag)
{
    uint32_t temp0[17],temp1[17],temp2[17];
    PassFlag tmp_flag = PASS;
    if(ecc_cmp_unsafe(X, P, wnum) >= 0)
    {
        tmp_flag = FAIL;
    }else{
      if(ecc_cmp_unsafe(Y,P,wnum) >= 0)
      {
          tmp_flag = FAIL;
      }
    }

    if(tmp_flag == PASS){
      (void)ecc_modmult(P, Y, Y,wnum, bnum,temp0);/*Y^2*/
      (void)ecc_modmult(P, X, X,wnum, bnum,temp1);
      (void)ecc_modmult(P, X, temp1,wnum,bnum,temp2);/*X^3*/
      (void)ecc_modmult(P, A, X, wnum,bnum,temp1);/*A*X*/
      (void)ecc_modadd(P,temp2,temp1,wnum,bnum,temp1);/*X^3 + A*X*/
      (void)ecc_modadd(P,temp1,B,wnum,bnum,temp2);/*X^3 + A*X +B*/
      if(0 != ecc_cmp_unsafe(temp0,temp2,wnum))
      {
          tmp_flag = FAIL;
      }
    }
    *ver_flag = tmp_flag;
    return SUCCESS;
}

/**
 * @brief ecc_modinv
 * @param P modular value
 * @param X value
 * @param wnum word length
 * @param bnum bit length
 *
 * @return result x^-1 mod P
 *
 */
static ErrStatus ecc_modinv(const uint32_t P[], const uint32_t U[], uint8_t wnum, uint16_t bnum, uint32_t result[], PassFlag *flag)
{
    ErrStatus tmp_err = SUCCESS;
    uint8_t i;
    for( i = 0U; i < wnum; i++)
    {
        SEA_ECC_MAP_CONST->REG[i] = 0U;
        SEA_ECC_MOD_P->REG[i] = P[i];
        SEA_ECC_MOD_PT->REG[i] = P[i];
        SEA_ECC_MAP_U->REG[i] = U[i];
        SEA_ECC_MAP_UT->REG[i] = U[i];
    }
    SEA_ECC_MAP_CONST->REG[0] = 1U;
    SEA_CYPHER->NLen = bnum;

    SEA_ECC_IR_FUNC_RUN(SEA_CTRL_IR_ECC_ModInv, SEA_TIMEOUT_IR_ECC_ModInv, &tmp_err);

    if((SEA_CYPHER->Status & 2U)==0U){
        *flag =  FAIL;
    }else{
        *flag =  PASS;
    }

    for( i = 0U; i < wnum; i++)
    {
        result[i] = SEA_ECC_MAP_UZ->REG[i];
    }

    return tmp_err;
}
/**
 * @brief ecc_add_i2m
 * @param wnum word length
 * @param A curve parameter a
 * @param Q0_X vector Q0 x-coordinate
 * @param Q0_Y vector Q0 y-coordinate
 * @param Q1_X vector Q1 x-coordinate
 * @param Q1_Y vector Q1 y-coordinate
 *
 */
static ErrStatus ecc_add_i2m(uint8_t wnum, const uint32_t A[],const uint32_t Q0_X[], const uint32_t Q0_Y[], const uint32_t Q1_X[], const uint32_t Q1_Y[])
{
    uint8_t i;
    ErrStatus tmp_err = SUCCESS;
    SEA_ECC_MAP_CONST->REG[0] = 1U;
    SEA_ECC_MAP_Q0_Z->REG[0]  = 1U;
    SEA_ECC_MAP_Q1_Z->REG[0]  = 1U;
    for(i = 1U; i < wnum; i++)
    {
        SEA_ECC_MAP_CONST->REG[i] = 0U;
        SEA_ECC_MAP_Q0_Z->REG[i]  = 0U;
        SEA_ECC_MAP_Q1_Z->REG[i]  = 0U;
    }

    for(i = 0U; i < wnum; i++)
    {
        SEA_ECC_MAP_A->REG[i] = A[i];
        SEA_ECC_MAP_Q0_X->REG[i] = Q0_X[i];
        SEA_ECC_MAP_Q1_X->REG[i] = Q1_X[i];
        SEA_ECC_MAP_Q0_Y->REG[i] = Q0_Y[i];
        SEA_ECC_MAP_Q1_Y->REG[i] = Q1_Y[i];
    }

    SEA_ECC_IR_FUNC_RUN(SEA_CTRL_IR_ECC_ADD_I2M, SEA_TIMEOUT_IR_ECC_ADD_I2M, &tmp_err);

    return tmp_err;
}
/**
 * @brief ecc_infinity
 * @param wnum word length
 *
 * @return infinity whether the result is infinity or not
 */
static void ecc_infinity(const uint8_t wnum, PassFlag* infinity_status)
{
    const uint32_t const0[17] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

    (void)ecc_m2i();
    if(ecc_cmp_unsafe(const0,(uint32_t*)(SEA_EMB_BASEADDR + 0x090),wnum)>=0)
    {
        *infinity_status = FAIL;
    }
}

#ifdef ECC_CRYPT_ITEMS_IN_BYTES
static void str2word(const uint8_t *str, uint32_t *words, uint32_t bytelen) {
    uint32_t i, len;
    uint8_t *s, temp;

    s = (uint8_t *)words;
    if (bytelen != 66U) {
        len = bytelen;
        sea_memcpy(s, str, len);
    } else {
        len = (bytelen + 2U);
        sea_memcpy((uint8_t *)(s + 2), str, bytelen);
    }
    for (i = 0; i < (len >> 1U); i++) {
        temp = s[i];
        s[i] = s[len - 1 - i];
        s[len - 1 - i] = temp;
    }

}
#endif

/******************************************************************************
 * Public Interface
 *****************************************************************************/

/*!
    \brief calculating share key pairs in ECDH, outputs x and y coordinates.
    \param[in]  curve:     ECC curve's parameter
    \param[in]  pkey:      pointer to private key from A
    \param[in]  Q:         pointer to public key pairs from B
    \param[out] DHkey:     negotiate key including DHkey_x and DHkey_y
    \retval     ErrStatus: SUCCESS or ERROR
 */
ErrStatus LL_ECC_DHkeygen(const uECC_Curve_t *curve, const eccData_t *_Q, const eccData_t *_pkey, eccData_t *_DHkey) {
    ErrStatus tmp_err = SUCCESS;
    const uint32_t *pkey;
    const uint32_t *Q;
    uint32_t *DHkey;

    sea_init_all();
    if ((_pkey == 0U) || (_Q == 0U) || (_DHkey == 0U)) {
        setErrorCode(ECC_PARASLEN_FAILURE);
    }
#ifdef ECC_CRYPT_ITEMS_IN_BYTES
    uint32_t tmp_pkey[17], tmp_Q[34], tmp_DHkey[34];

    sea_memset_word(tmp_pkey, 0U, 17U);
    sea_memset_word(tmp_Q, 0U, 34U);
    sea_memset_word(tmp_DHkey, 0U, 34U);
    str2word((const uint8_t *)_pkey, tmp_pkey, curve->num_bytes);
    str2word((const uint8_t *)_Q, tmp_Q, curve->num_bytes);
    str2word((const uint8_t *)((uint32_t)_Q + curve->num_bytes), (uint32_t *)(uint32_t)&tmp_Q[curve->num_words], curve->num_bytes);
    pkey = tmp_pkey;
    Q = tmp_Q;
    DHkey = tmp_DHkey;
#else
    pkey = (uint32_t *)_pkey;
    Q = (uint32_t *)_Q;
    DHkey = (uint32_t *)_DHkey;
#endif

    (void)ecc_point_mult(curve->p, curve->a, pkey, Q, (uint32_t *)(uint32_t)&Q[curve->num_words], curve->num_words, curve->num_bits, DHkey, (uint32_t *)(uint32_t)&DHkey[curve->num_words]);
    if (CYPHER_OK != checkErrors()) {
        tmp_err = ERROR;
    }

#ifdef ECC_CRYPT_ITEMS_IN_BYTES
    str2word((const uint8_t *)DHkey, (uint32_t *)_DHkey, curve->num_bytes);
    str2word((const uint8_t *)(uint32_t)&DHkey[curve->num_words], (uint32_t *)((uint32_t)_DHkey + curve->num_bytes), curve->num_bytes);
#endif

    return tmp_err;
}

/*!
    \brief the x coordinate is used as the basis for ECDH. Reference Specification NIST SP 800-56A and SEC 1.
    \param[in]  curve:     ECC curve's parameter
    \param[in]  pkey:      pointer to private key from A
    \param[in]  Q:         pointer to public key pairs from B
    \param[out] DHkey:     pointer to DHkey_x
    \retval ErrStatus:     SUCCESS or ERROR
 */
ErrStatus LL_ECC_DHkeygen_x(const uECC_Curve_t *curve, const eccData_t *_Q, const eccData_t *_pkey, eccData_t *_DHkey) {
    ErrStatus tmp_err = SUCCESS;
    const uint32_t *pkey;
    const uint32_t *Q;
    uint32_t *DHkey, tmp_DHkey[34];

    sea_init_all();
    if ((_pkey == 0U) || (_Q == 0U) || (_DHkey == 0U)) {
        setErrorCode(ECC_PARASLEN_FAILURE);
    }
    sea_memset_word(tmp_DHkey, 0U, 34U);
#ifdef ECC_CRYPT_ITEMS_IN_BYTES
    uint32_t tmp_pkey[17], tmp_Q[34];

    sea_memset_word(tmp_pkey, 0U, 17U);
    sea_memset_word(tmp_Q, 0U, 34U);
    str2word((const uint8_t *)_pkey, tmp_pkey, curve->num_bytes);
    str2word((const uint8_t *)_Q, tmp_Q, curve->num_bytes);
    str2word((const uint8_t *)((uint32_t)_Q + curve->num_bytes), (uint32_t *)(uint32_t)&tmp_Q[curve->num_words], curve->num_bytes);

    pkey = tmp_pkey;
    Q = tmp_Q;
#else
    pkey = (uint32_t *)_pkey;
    Q = (uint32_t *)_Q;
#endif
    DHkey = (uint32_t *)tmp_DHkey;

    (void)ecc_point_mult(curve->p, curve->a, pkey, Q, (uint32_t *)(uint32_t)&Q[curve->num_words], curve->num_words, curve->num_bits, DHkey, (uint32_t *)(uint32_t)&DHkey[curve->num_words]);
    if (CYPHER_OK != checkErrors()) {
        tmp_err = ERROR;
    }

#ifdef ECC_CRYPT_ITEMS_IN_BYTES
    str2word((const uint8_t *)DHkey, (uint32_t *)_DHkey, curve->num_bytes); /* only save DHkey_x */
#else
    sea_memcpy_word((uint32_t *)_DHkey, DHkey, curve->num_words);     /* only save DHkey_x */
#endif

    return tmp_err;
}

/*!
    \brief generating ECC private key and public key pairs
    \param[in]  curve:     ECC curve's parameter
    \param[out] pkey:      pointer to private key
    \param[out] Q:         pointer to public key pairs
    \retval     ErrStatus: SUCCESS or ERROR
 */
ErrStatus LL_ECC_keygen(const uECC_Curve_t *curve, eccData_t *_Q, eccData_t *_pkey) {
    ErrStatus tmp_err = SUCCESS;
    uint32_t random[17], const0[17];
    uint32_t *pkey, *Q;

    clrErrorCode(0xFF);
    sea_init_all();
    if ((_pkey == 0U) || (_Q == 0U)) {
        setErrorCode(ECC_PARASLEN_FAILURE);
    }
#ifdef ECC_CRYPT_ITEMS_IN_BYTES
    uint32_t tmp_pkey[17], tmp_Q[34];
    sea_memset_word(tmp_pkey, 0U, 17U);
    sea_memset_word(tmp_Q, 0U, 34U);

    pkey = tmp_pkey;
    Q = tmp_Q;
#else
    pkey = (uint32_t *)_pkey;
    Q = (uint32_t *)_Q;
#endif
    sea_memset_word(const0, 0U, 17U);

    LL_TRNG_gen(random, curve->num_words);
    (void)ecc_modadd(curve->n, random, const0, curve->num_words, curve->num_bits, pkey);
    (void)ecc_point_mult(curve->p, curve->a, pkey, curve->G, (uint32_t *)(uint32_t)&curve->G[17], curve->num_words, curve->num_bits, Q, (uint32_t *)(uint32_t)&Q[curve->num_words]);
    if (CYPHER_OK != checkErrors()) {
        tmp_err = ERROR;
    }

    if (curve->num_bits == 521U) {
        pkey[curve->num_words - 1U] &= 0x1FFU; /* output 521bits key */
    }

#ifdef ECC_CRYPT_ITEMS_IN_BYTES
    str2word((const uint8_t *)pkey, (uint32_t *)_pkey, curve->num_bytes);
    str2word((const uint8_t *)Q, (uint32_t *)_Q, curve->num_bytes);
    str2word((const uint8_t *)(uint32_t)&Q[curve->num_words], (uint32_t *)((uint32_t)_Q + curve->num_bytes), curve->num_bytes);
#endif

    return tmp_err;
}

/*!
    \brief generating ECC public key pairs by using private key
    \param[in]  curve:     ECC curve's parameter
    \param[in]  pkey:      pointer to private key
    \param[out] Q:         pointer to public key pairs
    \retval     ErrStatus: SUCCESS or ERROR
 */
ErrStatus LL_ECC_public_keygen(const uECC_Curve_t *curve, eccData_t *_Q, const eccData_t *_pkey) {
    ErrStatus tmp_err = SUCCESS;
    const uint32_t *pkey;
    uint32_t *Q;

    clrErrorCode(0xFF);
    sea_init_all();

    if ((_pkey == 0U) || (_Q == 0U)) {
        setErrorCode(ECC_PARASLEN_FAILURE);
    }
#ifdef ECC_CRYPT_ITEMS_IN_BYTES
    uint32_t tmp_pkey[17], tmp_Q[34];

    sea_memset_word(tmp_pkey, 0U, 17U);
    sea_memset_word(tmp_Q, 0U, 34U);
    str2word((const uint8_t *)_pkey, tmp_pkey, curve->num_bytes);

    pkey = tmp_pkey;
    Q = tmp_Q;
#else
    pkey = (uint32_t *)_pkey;
    Q = (uint32_t *)_Q;
#endif

    if (ecc_cmp_unsafe(pkey, curve->p, curve->num_words) == -1) {
        (void)ecc_point_mult(curve->p, curve->a, pkey, curve->G, (uint32_t *)(uint32_t)&curve->G[17], curve->num_words, curve->num_bits, Q, (uint32_t *)(uint32_t)&Q[curve->num_words]);
        if (CYPHER_OK != checkErrors()) {
            tmp_err = ERROR;
        }
    } else {
        setErrorCode(ECC_PKEY_FAILURE);
        tmp_err = ERROR;
    }

#ifdef ECC_CRYPT_ITEMS_IN_BYTES
    str2word((const uint8_t *)Q, (uint32_t *)_Q, curve->num_bytes);
    str2word((const uint8_t *)(uint32_t)&Q[curve->num_words], (uint32_t *)((uint32_t)_Q + curve->num_bytes), curve->num_bytes);
#endif

    return tmp_err;
}

/*!
    \brief generating uncompressed public key pairs using private key. Reference Specification RFC-5480.
    \param[in]  curve:     ECC curve's parameter
    \param[in]  pkey:      pointer to private key
    \param[in]  Q:         pointer to public key pairs
    \retval     ErrStatus: SUCCESS or ERROR
 */
ErrStatus LL_ECC_public_keygen_uncompressed(const uECC_Curve_t *curve, eccData_t *_Q, const eccData_t *_pkey) {
    ErrStatus tmp_err = SUCCESS;

    clrErrorCode(0xFF);
    sea_init_all();

    if ((_pkey == 0U) || (_Q == 0U)) {
        setErrorCode(ECC_PARASLEN_FAILURE);
    }
#ifdef ECC_CRYPT_ITEMS_IN_BYTES
    const uint32_t *pkey;
    uint32_t *Q;
    uint32_t tmp_pkey[17], tmp_Q[34];

    sea_memset_word(tmp_pkey, 0U, 17U);
    sea_memset_word(tmp_Q, 0U, 34U);
    str2word((const uint8_t *)_pkey, tmp_pkey, curve->num_bytes);

    pkey = tmp_pkey;
    Q = tmp_Q;

    if (ecc_cmp_unsafe(pkey, curve->p, curve->num_words) == -1) {
        (void)ecc_point_mult(curve->p, curve->a, pkey, curve->G, (uint32_t *)(uint32_t)&curve->G[17], curve->num_words, curve->num_bits, Q, (uint32_t *)(uint32_t)&Q[curve->num_words]);
        if (CYPHER_OK != checkErrors()) {
            tmp_err = ERROR;
        }
    } else {
        setErrorCode(ECC_PKEY_FAILURE);
        tmp_err = ERROR;
    }

    str2word((const uint8_t *)Q, (uint32_t *)((uint32_t)_Q + 1U), curve->num_bytes);
    str2word((const uint8_t *)(uint32_t)&Q[curve->num_words], (uint32_t *)((uint32_t)_Q + curve->num_bytes + 1U), curve->num_bytes);
    sea_memset((uint8_t *)_Q, ECC_PUBKEY_HEADER_BYTE, 1U); /* uncompressed public key pairs header */
#else
    tmp_err = ERROR; /* uncompressed public key pairs only support byte array */
#endif

    return tmp_err;
}

/*!
    \brief checking ECC public key pairs is valid
    \param[in]  curve:     ECC curve's parameter
    \param[in]  Q:         pointer to public key pairs
    \param[out] ver_flag:  PASS-0, FAIL-1
    \retval     ErrStatus: SUCCESS or ERROR
 */
ErrStatus LL_ECC_keyver(const uECC_Curve_t *curve, const eccData_t *_Q, PassFlag *ver_flag) {
    ErrStatus tmp_err = SUCCESS;
    const uint32_t *Q;

    clrErrorCode(0xFF);
    sea_init_all();
    if ((_Q == 0U) || (ver_flag == 0U)) {
        setErrorCode(ECC_PARASLEN_FAILURE);
    }
#ifdef ECC_CRYPT_ITEMS_IN_BYTES
    uint32_t tmp_Q[34];

    sea_memset_word(tmp_Q, 0U, 34U);
    str2word((const uint8_t *)_Q, tmp_Q, curve->num_bytes);
    str2word((const uint8_t *)((uint32_t)_Q + curve->num_bytes), (uint32_t *)(uint32_t)&tmp_Q[curve->num_words], curve->num_bytes);

    Q = tmp_Q;
#else
    Q = (uint32_t *)_Q;
#endif
    (void)ecc_point_oncurve(curve->p, curve->a, curve->b, Q, (uint32_t *)(uint32_t)&Q[curve->num_words], curve->num_words, curve->num_bits, ver_flag);
    if (CYPHER_OK != checkErrors()) {
        tmp_err = ERROR;
    }

    return tmp_err;
}

/*!
    \brief generating public key pairs, private key and corresponding signatures based on the input hash result
    \param[in]  curve:      ECC curve's parameter
    \param[in]  Hm:         pointer to hash result
    \param[in]  Hm_len:     hash result length in bytes
    \param[out] pkey:       pointer to private key
    \param[out] Q:          pointer to public key pairs
    \param[out] r:          pointer to r of the signature pairs
    \param[out] s:          pointer to s of the signature pairs
    \retval     ErrStatus:  SUCCESS or ERROR
 */
ErrStatus LL_ECC_sign_gen(const uECC_Curve_t *curve, const eccData_t *_Hm, const uint32_t Hm_len, eccData_t *_Q, eccData_t *_pkey, eccData_t *_r, eccData_t *_s) {
    PassFlag tmp_flag = PASS;
    ErrStatus tmp_err = SUCCESS;
    uint32_t i, const0[17], resultX0[17], resultY0[17], k[17], k_inv[17];
    const uint32_t *Hm;
    const uint32_t *pkey;
    uint32_t *r, *s;

    clrErrorCode(0xFF);
    sea_init_all();

    if ((_pkey == 0U) || (_Q == 0U) || (_Hm == 0U) || (_r == 0U) || (_s == 0U) || (Hm_len == 0U)) {
        setErrorCode(ECC_PARASLEN_FAILURE);
    }

    sea_memset_word(const0, 0U, 17U);
    sea_memset_word(k, 0U, 17U);
    sea_memset_word(k_inv, 0U, 17U);
    sea_memset_word(resultX0, 0U, 17U);
    sea_memset_word(resultY0, 0U, 17U);
    (void)LL_ECC_keygen(curve, _Q, _pkey); /* generate private key and public key pairs */
#ifdef ECC_CRYPT_ITEMS_IN_BYTES
    uint32_t tmp_pkey[17], tmp_r[17], tmp_s[17];

    sea_memset_word(tmp_pkey, 0U, 17U);
    str2word((const uint8_t *)_pkey, tmp_pkey, curve->num_bytes);
    sea_memset_word(tmp_r, 0U, 17U);
    sea_memset_word(tmp_s, 0U, 17U);

    Hm = k;
    pkey = tmp_pkey;
    r = tmp_r;
    s = tmp_s;
#else
    Hm = (uint32_t *)_Hm;
    pkey = (uint32_t *)_pkey;
    r = (uint32_t *)_r;
    s = (uint32_t *)_s;
#endif

    LL_TRNG_gen(k, curve->num_words);
    (void)ecc_modadd(curve->n, k, const0, curve->num_words, curve->num_bits, k); /* k */
    if (CYPHER_OK == checkErrors()) {
        for (i = 0U; i < 10U; i++) {
            if (SUCCESS != ecc_modinv(curve->n, k, curve->num_words, curve->num_bits, k_inv, &tmp_flag)) { /* k^-1 */
                tmp_err = ERROR;
            } else {
                if (PASS != tmp_flag) {
                    if (1U == (k[0] & 1U)) {
                        k[0] -= 2U;
                    } else {
                        k[0] -= 1U;
                    }
                }
            }
            if ((SUCCESS != tmp_err) || (PASS == tmp_flag)) {
                break;
            }
        }
        if (i >= 10U) {
            setErrorCode(ECC_LOOPTIMETOUT_FAILURE);
            tmp_err = ERROR;
        }

        if ((CYPHER_OK == checkErrors()) && (PASS == tmp_flag)) {
            (void)ecc_point_mult(curve->p, curve->a, k, curve->G, (uint32_t *)(uint32_t)&curve->G[17U], curve->num_words, curve->num_bits, resultX0, resultY0); /* k*G */
            (void)ecc_modadd(curve->n, resultX0, const0, curve->num_words, curve->num_bits, r);                                                                 /* resultX0(mod N) */
            (void)ecc_modmult(curve->n, pkey, r, curve->num_words, curve->num_bits, resultY0);                                                                  /* d*r */
#ifdef ECC_CRYPT_ITEMS_IN_BYTES
            sea_memset_word(k, 0U, curve->num_words);
            str2word(_Hm, k, Hm_len); /* temporarily save Hm to k */
#endif
            (void)ecc_modadd(curve->n, Hm, resultY0, curve->num_words, curve->num_bits, resultX0);                                                              /* Hm+d*r */
            (void)ecc_modmult(curve->n, k_inv, resultX0, curve->num_words, curve->num_bits, s);                                                                 /* k^-1*(Hm+d*r) */
        }
    }
    if (CYPHER_OK != checkErrors()) {
        tmp_err = ERROR;
    }

#ifdef ECC_CRYPT_ITEMS_IN_BYTES
    str2word((const uint8_t *)r, (uint32_t *)_r, curve->num_bytes);
    str2word((const uint8_t *)s, (uint32_t *)_s, curve->num_bytes);
#endif

    return tmp_err;
}

/*!
    \brief generating public key pairs if the pointer of Q is not NULL, and corresponding signatures based on the input hash result
    \param[in]  curve:      ECC curve's parameter
    \param[in]  Hm:         pointer to hash result
    \param[in]  Hm_len:     hash result length in bytes
    \param[in]  pkey:       pointer to private key
    \param[out] Q:          pointer to public key pairs
    \param[out] r:          pointer to r of the signature pairs
    \param[out] s:          pointer to s of the signature pairs
    \retval     ErrStatus:  SUCCESS or ERROR
 */
ErrStatus LL_ECC_specify_pkey_sign_gen(const uECC_Curve_t *curve, const eccData_t *_Hm, const uint32_t Hm_len, eccData_t *_Q, eccData_t *_pkey, eccData_t *_r, eccData_t *_s) {
    PassFlag tmp_flag = PASS;
    ErrStatus tmp_err = SUCCESS;
    uint32_t i, const0[17], resultX0[17], resultY0[17], k[17], k_inv[17];
    const uint32_t *Hm;
    const uint32_t *pkey;
    uint32_t *r, *s;

    clrErrorCode(0xFF);
    sea_init_all();
    if ((_pkey == 0U) || (_Hm == 0U) || (_r == 0U) || (_s == 0U) || (Hm_len == 0U)) {
        setErrorCode(ECC_PARASLEN_FAILURE);
    }

    sea_memset_word(const0, 0U, 17U);
    sea_memset_word(k, 0U, 17U);
    sea_memset_word(k_inv, 0U, 17U);
    sea_memset_word(resultX0, 0U, 17U);
    sea_memset_word(resultY0, 0U, 17U);
    if (_Q != 0U) {                                    /* if the public key pairs is not empty */
        (void)LL_ECC_public_keygen(curve, _Q, _pkey);  /* generate public key pairs */
    }
#ifdef ECC_CRYPT_ITEMS_IN_BYTES
    uint32_t tmp_pkey[17], tmp_r[17], tmp_s[17];

    sea_memset_word(tmp_pkey, 0U, 17U);
    str2word((const uint8_t *)_pkey, tmp_pkey, curve->num_bytes);
    sea_memset_word(tmp_r, 0U, 17U);
    sea_memset_word(tmp_s, 0U, 17U);

    Hm = k;
    pkey = tmp_pkey;
    r = tmp_r;
    s = tmp_s;
#else
    Hm = (uint32_t *)_Hm;
    pkey = (uint32_t *)_pkey;
    r = (uint32_t *)_r;
    s = (uint32_t *)_s;
#endif

    LL_TRNG_gen(k, curve->num_words);
    (void)ecc_modadd(curve->n, k, const0, curve->num_words, curve->num_bits, k); /* k */
    if (CYPHER_OK == checkErrors()) {
        for (i = 0U; i < 10U; i++) {
            if (SUCCESS != ecc_modinv(curve->n, k, curve->num_words, curve->num_bits, k_inv, &tmp_flag)) { /* k^-1 */
                tmp_err = ERROR;
            } else {
                if (PASS != tmp_flag) {
                    if (1U == (k[0] & 1U)) {
                        k[0] -= 2U;
                    } else {
                        k[0] -= 1U;
                    }
                }
            }
            if ((SUCCESS != tmp_err) || (PASS == tmp_flag)) {
                break;
            }
        }
        if (i >= 10U) {
            setErrorCode(ECC_LOOPTIMETOUT_FAILURE);
            tmp_err = ERROR;
        }

        if ((CYPHER_OK == checkErrors()) && (PASS == tmp_flag)) {
            (void)ecc_point_mult(curve->p, curve->a, k, curve->G, (uint32_t *)(uint32_t)&curve->G[17U], curve->num_words, curve->num_bits, resultX0, resultY0); /* k*G */
            (void)ecc_modadd(curve->n, resultX0, const0, curve->num_words, curve->num_bits, r);                                                                 /* resultX0(mod N) */
            (void)ecc_modmult(curve->n, pkey, r, curve->num_words, curve->num_bits, resultY0);                                                                  /* d*r */
#ifdef ECC_CRYPT_ITEMS_IN_BYTES
            sea_memset_word(k, 0U, curve->num_words);
            str2word(_Hm, k, Hm_len); /* temporarily save Hm to k */
#endif
            (void)ecc_modadd(curve->n, Hm, resultY0, curve->num_words, curve->num_bits, resultX0);                                                              /* Hm+d*r */
            (void)ecc_modmult(curve->n, k_inv, resultX0, curve->num_words, curve->num_bits, s);                                                                 /* k^-1*(Hm+d*r) */
        }
    }
    if (CYPHER_OK != checkErrors()) {
        tmp_err = ERROR;
    }

#ifdef ECC_CRYPT_ITEMS_IN_BYTES
    str2word((const uint8_t *)r, (uint32_t *)_r, curve->num_bytes);
    str2word((const uint8_t *)s, (uint32_t *)_s, curve->num_bytes);
#endif

    return tmp_err;
}

/*!
    \brief verifing the input signature by using public key based on the input hash result
    \param[in]  curve:     ECC curve's parameter
    \param[in]  Hm:        pointer to hash result
    \param[in]  Hm_len:    hash result length in bytes
    \param[in]  Q:         pointer to public key pairs
    \param[in]  r:         pointer to r of the signature pairs
    \param[in]  s:         pointer to s of the signature pairs
    \param[out] ver_flag:  PASS-0,FAIL-1
    \retval     ErrStatus: SUCCESS or ERROR
 */
ErrStatus LL_ECC_sign_ver(const uECC_Curve_t *curve, const eccData_t *_Hm, const uint32_t Hm_len, const eccData_t *_Q, const eccData_t *_r, const eccData_t *_s, PassFlag *ver_flag) {
    PassFlag infinity_status = PASS;
    ErrStatus tmp_err = SUCCESS;
    uint32_t i, const0[17], resultX0[17], resultY0[17], resultX1[17], resultY1[17], resultX2[17];
    int tmp_cmp_sta[4];
    const uint32_t *Hm;
    const uint32_t *Q;
    const uint32_t *r;
    const uint32_t *s;

    clrErrorCode(0xFF);
    sea_init_all();
    if ((_Q == 0U) || (_Hm == 0U) || (_r == 0U) || (_s == 0U) || (Hm_len == 0U)) {
        setErrorCode(ECC_PARASLEN_FAILURE);
    }
#ifdef ECC_CRYPT_ITEMS_IN_BYTES
    uint32_t tmp_Q[34], tmp_r[17], tmp_s[17];

    sea_memset_word(tmp_Q, 0U, 34U);
    sea_memset_word(tmp_r, 0U, 17U);
    sea_memset_word(tmp_s, 0U, 17U);
    str2word((const uint8_t *)_Q, tmp_Q, curve->num_bytes);
    str2word((const uint8_t *)((uint32_t)_Q + curve->num_bytes), (uint32_t *)(uint32_t)&tmp_Q[curve->num_words], curve->num_bytes);
    str2word((const uint8_t *)_r, tmp_r, curve->num_bytes);
    str2word((const uint8_t *)_s, tmp_s, curve->num_bytes);

    Hm = resultX2; /* temporarily save Hm to resultX2 */
    Q = tmp_Q;
    r = tmp_r;
    s = tmp_s;
#else
    Hm = (uint32_t *)_Hm;
    Q = (uint32_t *)_Q;
    r = (uint32_t *)_r;
    s = (uint32_t *)_s;
#endif

    sea_memset_word(const0, 0U, 17U);
    sea_memset_word(resultX0, 0U, 17U);
    sea_memset_word(resultX1, 0U, 17U);
    sea_memset_word(resultX2, 0U, 17U);
    sea_memset_word(resultY0, 0U, 17U);
    sea_memset_word(resultY1, 0U, 17U);
    (void)ecc_point_oncurve(curve->p, curve->a, curve->b, Q, (uint32_t *)(uint32_t)&Q[curve->num_words], curve->num_words, curve->num_bits, &infinity_status);
    if ((CYPHER_OK == checkErrors()) && (PASS == infinity_status)) {
        tmp_cmp_sta[0] = ecc_cmp_unsafe(r, curve->n, curve->num_words);
        tmp_cmp_sta[1] = ecc_cmp_unsafe(const0, r, curve->num_words);
        tmp_cmp_sta[2] = ecc_cmp_unsafe(s, curve->n, curve->num_words);
        tmp_cmp_sta[3] = ecc_cmp_unsafe(const0, s, curve->num_words);

        if ((tmp_cmp_sta[0] >= 0) || (tmp_cmp_sta[1] >= 0) || (tmp_cmp_sta[2] >= 0) || (tmp_cmp_sta[3] >= 0)) {
            infinity_status = FAIL;
        } else {
            if (SUCCESS != ecc_modinv(curve->n, s, curve->num_words, curve->num_bits, resultX0, &infinity_status)) { /* s^-1 */
                tmp_err = ERROR;
            }

            if ((CYPHER_OK == checkErrors()) && (PASS == infinity_status)) {
#ifdef ECC_CRYPT_ITEMS_IN_BYTES
                str2word((const uint8_t *)_Hm, resultX2, Hm_len); /* temporarily save Hm to resultX2 */
#endif
                (void)ecc_modmult(curve->n, Hm, resultX0, curve->num_words, curve->num_bits, resultX1); /* u1 = s^-1*Hm */
                (void)ecc_modmult(curve->n, r, resultX0, curve->num_words, curve->num_bits, resultX2);  /* u2 = s^-1*r */
                (void)ecc_point_mult(curve->p, curve->a, resultX1, curve->G, (uint32_t *)(uint32_t)&curve->G[17], curve->num_words, curve->num_bits, resultX0, resultY0); /* u1*G */
                (void)ecc_point_mult(curve->p, curve->a, resultX2, Q, (uint32_t *)(uint32_t)&Q[curve->num_words], curve->num_words, curve->num_bits, resultX1, resultY1); /* u2*Q */
                (void)ecc_add_i2m(curve->num_words, curve->a, resultX0, resultY0, resultX1, resultY1);
                (void)ecc_point_add();
                ecc_infinity((uint8_t)curve->num_words, &infinity_status);

                if ((CYPHER_OK == checkErrors()) && (infinity_status == PASS)) {
                    for (i = 0U; i < curve->num_words; i++) {
                        resultX2[i] = SEA_ECC_MAP_Q0_X->REG[i];
                    }

                    if (SUCCESS == ecc_modadd(curve->n, const0, resultX2, curve->num_words, curve->num_bits, resultX1)) {
                        for (i = 0U; i < curve->num_words; i++) {
                            if (resultX1[i] != r[i]) {
                                infinity_status = FAIL;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
    if (PASS != infinity_status) {
        *ver_flag = FAIL;
    } else {
        *ver_flag = PASS;
    }

    return tmp_err;
}

#else
ErrStatus LL_ECC_DHkeygen(const uECC_Curve_t *curve, const eccData_t *_Q, const eccData_t *_pkey, eccData_t *_DHkey)
{
    uint32_t iap_addr;
    if (IAP_PASS != getIAPHeaderAddress(&iap_addr)) {
        return SEA_IAP_FAILURE;
    }
    SEA_ApiTypeDef *Iap_Func = (SEA_ApiTypeDef *)iap_addr;
    pLL_ECC_DHkeygen func_ptr = (pLL_ECC_DHkeygen)((uint32_t)Iap_Func->LL_ECC_DHkeygen + ROM_BASE_ADDR);
    return func_ptr(curve, _Q, _pkey, _DHkey);
}

ErrStatus LL_ECC_DHkeygen_x(const uECC_Curve_t *curve, const eccData_t *_Q, const eccData_t *_pkey, eccData_t *_DHkey)
{
    uint32_t DHKey_tmp[34] = {0};
    uint32_t ret = LL_ECC_DHkeygen(curve, _Q, _pkey, (uint8_t *)DHKey_tmp);
    for (int i = 0; i < curve->num_bytes; i++) {
        _DHkey[i] = ((uint8_t*)DHKey_tmp)[i];
    }
    return ret;
}

ErrStatus LL_ECC_keygen(const uECC_Curve_t *curve, eccData_t *_Q, eccData_t *_pkey)
{
    uint32_t iap_addr;
    if (IAP_PASS != getIAPHeaderAddress(&iap_addr)) {
        return SEA_IAP_FAILURE;
    }
    SEA_ApiTypeDef *Iap_Func = (SEA_ApiTypeDef *)iap_addr;
    pLL_ECC_keygen func_ptr = (pLL_ECC_keygen)((uint32_t)Iap_Func->LL_ECC_keygen + ROM_BASE_ADDR);
    return func_ptr(curve, _Q, _pkey);
}

ErrStatus LL_ECC_public_keygen(const uECC_Curve_t *curve, eccData_t *_Q, const eccData_t *_pkey)
{
    uint32_t iap_addr;
    if (IAP_PASS != getIAPHeaderAddress(&iap_addr)) {
        return SEA_IAP_FAILURE;
    }
    SEA_ApiTypeDef *Iap_Func = (SEA_ApiTypeDef *)iap_addr;
    pLL_ECC_public_keygen func_ptr = (pLL_ECC_public_keygen)((uint32_t)Iap_Func->LL_ECC_public_keygen + ROM_BASE_ADDR);
    return func_ptr(curve, _Q, _pkey);
}

ErrStatus LL_ECC_public_keygen_uncompressed(const uECC_Curve_t *curve, eccData_t *_Q, const eccData_t *_pkey)
{
    uint32_t Q_tmp[34] = {0};
    uint32_t ret = LL_ECC_public_keygen(curve, (uint8_t *)Q_tmp, _pkey);
    for (int i = 0; i < curve->num_bytes; i++) {
        _Q[i + 1] = ((uint8_t*)Q_tmp)[i];
    }
    *_Q = 0x04U;
    return ret;
}

ErrStatus LL_ECC_keyver(const uECC_Curve_t *curve, const eccData_t *_Q, PassFlag *ver_flag)
{
    uint32_t iap_addr;
    if (IAP_PASS != getIAPHeaderAddress(&iap_addr)) {
        return SEA_IAP_FAILURE;
    }
    SEA_ApiTypeDef *Iap_Func = (SEA_ApiTypeDef *)iap_addr;
    pLL_ECC_keyver func_ptr = (pLL_ECC_keyver)((uint32_t)Iap_Func->LL_ECC_keyver + ROM_BASE_ADDR);
    return func_ptr(curve, _Q, ver_flag);
}

ErrStatus LL_ECC_sign_gen(const uECC_Curve_t *curve, const eccData_t *_Hm, const uint32_t Hm_len, eccData_t *_Q, eccData_t *_pkey, eccData_t *_r, eccData_t *_s)
{
    uint32_t iap_addr;
    if (IAP_PASS != getIAPHeaderAddress(&iap_addr)) {
        return SEA_IAP_FAILURE;
    }
    SEA_ApiTypeDef *Iap_Func = (SEA_ApiTypeDef *)iap_addr;
    pLL_ECC_sign_gen func_ptr = (pLL_ECC_sign_gen)((uint32_t)Iap_Func->LL_ECC_sign_gen + ROM_BASE_ADDR);
    return func_ptr(curve, _Hm, Hm_len, _Q, _pkey, _r, _s);
}

ErrStatus LL_ECC_specify_pkey_sign_gen(const uECC_Curve_t *curve, const eccData_t *_Hm, const uint32_t Hm_len, eccData_t *_Q, eccData_t *_pkey, eccData_t *_r, eccData_t *_s)
{
    uint32_t iap_addr;
    if (IAP_PASS != getIAPHeaderAddress(&iap_addr)) {
        return SEA_IAP_FAILURE;
    }
    SEA_ApiTypeDef *Iap_Func = (SEA_ApiTypeDef *)iap_addr;
    pLL_ECC_specify_pkey_sign_gen func_ptr = (pLL_ECC_specify_pkey_sign_gen)((uint32_t)Iap_Func->LL_ECC_specify_pkey_sign_gen + ROM_BASE_ADDR);
    return func_ptr(curve, _Hm, Hm_len, _Q, _pkey, _r, _s);
}

ErrStatus LL_ECC_sign_ver(const uECC_Curve_t *curve, const eccData_t *_Hm, const uint32_t Hm_len, const eccData_t *_Q, const eccData_t *_r, const eccData_t *_s, PassFlag *ver_flag)
{
    uint32_t iap_addr;
    if (IAP_PASS != getIAPHeaderAddress(&iap_addr)) {
        return SEA_IAP_FAILURE;
    }
    SEA_ApiTypeDef *Iap_Func = (SEA_ApiTypeDef *)iap_addr;
    pLL_ECC_sign_ver func_ptr = (pLL_ECC_sign_ver)((uint32_t)Iap_Func->LL_ECC_sign_ver + ROM_BASE_ADDR);
    return func_ptr(curve, _Hm, Hm_len, _Q, _r, _s, ver_flag);
}

ErrStatus LL_ECC_keygen_sec(const uECC_Curve_t *curve, uint8_t pkeyIndex, uint8_t QxIndex, uint8_t QyIndex)
{
    uint32_t iap_addr;
    if (IAP_PASS != getIAPHeaderAddress(&iap_addr)) {
        return SEA_IAP_FAILURE;
    }
    SEA_ApiTypeDef *Iap_Func = (SEA_ApiTypeDef *)iap_addr;
    pLL_ECC_keygen_sec func_ptr = (pLL_ECC_keygen_sec)((uint32_t)Iap_Func->LL_ECC_keygen_sec + ROM_BASE_ADDR);
    return func_ptr(curve, pkeyIndex, QxIndex, QyIndex);
}

ErrStatus LL_ECC_sign_gen_sec(const uECC_Curve_t *curve, const eccData_t *_Hm, const uint32_t Hm_len, const uint8_t pkeyIndex, eccData_t *_r, eccData_t *_s)
{
    uint32_t iap_addr;
    if (IAP_PASS != getIAPHeaderAddress(&iap_addr)) {
        return SEA_IAP_FAILURE;
    }
    SEA_ApiTypeDef *Iap_Func = (SEA_ApiTypeDef *)iap_addr;
    pLL_ECC_sign_gen_sec func_ptr = (pLL_ECC_sign_gen_sec)((uint32_t)Iap_Func->LL_ECC_sign_gen_sec + ROM_BASE_ADDR);
    return func_ptr(curve, _Hm, Hm_len, pkeyIndex, _r, _s);
}

ErrStatus LL_ECC_sign_ver_sec(const uECC_Curve_t *curve, const eccData_t *_Hm, const uint32_t Hm_len, const uint8_t QxIndex, const uint8_t QyIndex, const eccData_t *_r, const eccData_t *_s, PassFlag *ver_flag)
{
    uint32_t iap_addr;
    if (IAP_PASS != getIAPHeaderAddress(&iap_addr)) {
        return SEA_IAP_FAILURE;
    }
    SEA_ApiTypeDef *Iap_Func = (SEA_ApiTypeDef *)iap_addr;
    pLL_ECC_sign_ver_sec func_ptr = (pLL_ECC_sign_ver_sec)((uint32_t)Iap_Func->LL_ECC_sign_ver_sec + ROM_BASE_ADDR);
    return func_ptr(curve, _Hm, Hm_len, QxIndex, QyIndex, _r, _s, ver_flag);
}
#endif
#endif
/* r2894 */
