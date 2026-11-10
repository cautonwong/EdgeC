/**
 * @file dicom_parser.c
 * @brief Micro-DICOM Frame & Telemetry Parser Implementation
 */

#include "dicom_parser.h"
#include <string.h>
#include <math.h>

int dicom_parse_header(const uint8_t *buf, size_t len, dicom_meta_t *meta) {
    if (!buf || !meta || len < 132) {
        return -1;
    }

    memset(meta, 0, sizeof(dicom_meta_t));

    /* Check DICOM prefix at offset 128 "DICM" */
    if (buf[128] != 'D' || buf[129] != 'I' || buf[130] != 'C' || buf[131] != 'M') {
        /* Support headerless micro-DICOM stream if DICM is absent */
        meta->pixel_data_offset = 0;
    } else {
        meta->pixel_data_offset = 132;
    }

    /* Parse basic Tag elements (Little Endian explicit/implicit) */
    size_t idx = meta->pixel_data_offset;
    while (idx + 4 <= len) {
        uint16_t group = (uint16_t)(buf[idx] | (buf[idx + 1] << 8));
        uint16_t elem = (uint16_t)(buf[idx + 2] | (buf[idx + 3] << 8));

        /* Image Pixel Module Tags (0x0028) */
        if (group == 0x0028) {
            if (elem == 0x0010 && idx + 10 <= len) { /* Rows */
                meta->rows = (uint16_t)(buf[idx + 8] | (buf[idx + 9] << 8));
            } else if (elem == 0x0011 && idx + 10 <= len) { /* Columns */
                meta->columns = (uint16_t)(buf[idx + 8] | (buf[idx + 9] << 8));
            } else if (elem == 0x0100 && idx + 10 <= len) { /* Bits Allocated */
                meta->bits_allocated = (uint16_t)(buf[idx + 8] | (buf[idx + 9] << 8));
            } else if (elem == 0x0101 && idx + 10 <= len) { /* Bits Stored */
                meta->bits_stored = (uint16_t)(buf[idx + 8] | (buf[idx + 9] << 8));
            }
        } else if (group == 0x7FE0 && elem == 0x0010) { /* Pixel Data */
            meta->pixel_data_offset = idx + 12;
            if (meta->pixel_data_offset < len) {
                meta->pixel_data_length = len - meta->pixel_data_offset;
            }
            break;
        }

        idx += 2;
    }

    /* Fallback defaults if unspecified */
    if (meta->rows == 0) meta->rows = 64;
    if (meta->columns == 0) meta->columns = 64;
    if (meta->bits_allocated == 0) meta->bits_allocated = 16;
    if (meta->pixel_data_length == 0 && meta->pixel_data_offset < len) {
        meta->pixel_data_length = len - meta->pixel_data_offset;
    }

    /* Analyze intensity & SNR over pixel buffer */
    if (meta->pixel_data_offset < len) {
        size_t total_pixels = meta->rows * meta->columns;
        size_t avail_pixels = meta->pixel_data_length / 2;
        if (avail_pixels < total_pixels) {
            total_pixels = avail_pixels;
        }

        if (total_pixels > 0) {
            const uint16_t *pixels = (const uint16_t *)(buf + meta->pixel_data_offset);
            double sum = 0.0;
            double min_val = 65535.0;
            double max_val = 0.0;

            for (size_t i = 0; i < total_pixels; i++) {
                double val = (double)pixels[i];
                sum += val;
                if (val < min_val) min_val = val;
                if (val > max_val) max_val = val;
            }

            meta->mean_intensity = sum / (double)total_pixels;
            meta->min_intensity = min_val;
            meta->max_intensity = max_val;

            /* Noise variance estimation */
            double sq_diff_sum = 0.0;
            for (size_t i = 0; i < total_pixels; i++) {
                double diff = (double)pixels[i] - meta->mean_intensity;
                sq_diff_sum += diff * diff;
            }
            double variance = sq_diff_sum / (double)total_pixels;
            double std_dev = sqrt(variance);

            if (std_dev > 1e-6) {
                meta->snr_db = 20.0 * log10(meta->mean_intensity / std_dev);
            } else {
                meta->snr_db = 60.0; /* Cap high SNR */
            }
        }
    }

    return 0;
}

int dicom_compute_roi_mean(const uint8_t *buf, size_t len, const dicom_meta_t *meta,
                           uint16_t roi_x, uint16_t roi_y,
                           uint16_t roi_w, uint16_t roi_h,
                           double *roi_mean) {
    if (!buf || !meta || !roi_mean || meta->pixel_data_offset >= len) {
        return -1;
    }

    if (roi_x + roi_w > meta->columns || roi_y + roi_h > meta->rows || roi_w == 0 || roi_h == 0) {
        return -2;
    }

    const uint16_t *pixels = (const uint16_t *)(buf + meta->pixel_data_offset);
    double sum = 0.0;
    size_t count = 0;

    for (uint16_t r = roi_y; r < roi_y + roi_h; r++) {
        for (uint16_t c = roi_x; c < roi_x + roi_w; c++) {
            size_t idx = (size_t)r * (size_t)meta->columns + (size_t)c;
            sum += (double)pixels[idx];
            count++;
        }
    }

    if (count > 0) {
        *roi_mean = sum / (double)count;
        return 0;
    }

    return -3;
}
