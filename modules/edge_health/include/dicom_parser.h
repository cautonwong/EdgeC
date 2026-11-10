/**
 * @file dicom_parser.h
 * @brief Micro-DICOM Frame & Telemetry Parser for Edge Medical Imaging
 */

#ifndef DICOM_PARSER_H
#define DICOM_PARSER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint16_t rows;
    uint16_t columns;
    uint16_t bits_allocated;
    uint16_t bits_stored;
    uint16_t high_bit;
    uint16_t pixel_representation;
    size_t pixel_data_offset;
    size_t pixel_data_length;
    double mean_intensity;
    double max_intensity;
    double min_intensity;
    double snr_db;
} dicom_meta_t;

/**
 * @brief Parses raw micro-DICOM stream buffer header metadata
 * @param buf Pointer to binary DICOM stream
 * @param len Size of buffer in bytes
 * @param meta Output metadata structure
 * @return 0 on success, negative error code on failure
 */
int dicom_parse_header(const uint8_t *buf, size_t len, dicom_meta_t *meta);

/**
 * @brief Computes Region of Interest (ROI) mean intensity
 * @param buf Raw DICOM buffer
 * @param len Size of buffer
 * @param meta Parsed DICOM metadata
 * @param roi_x ROI top-left X coordinate
 * @param roi_y ROI top-left Y coordinate
 * @param roi_w ROI width
 * @param roi_h ROI height
 * @param roi_mean Output ROI mean intensity
 * @return 0 on success, negative error code on failure
 */
int dicom_compute_roi_mean(const uint8_t *buf, size_t len, const dicom_meta_t *meta,
                           uint16_t roi_x, uint16_t roi_y,
                           uint16_t roi_w, uint16_t roi_h,
                           double *roi_mean);

#ifdef __cplusplus
}
#endif

#endif /* DICOM_PARSER_H */
