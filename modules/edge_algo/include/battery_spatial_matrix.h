/**
 * @file battery_spatial_matrix.h
 * @brief 4096-Node Cell Voltage-Temperature-Current Sliding Window Polarization Feature
 *        Extractor & Thermal Runaway Spatial Diffusion Predictor (AEP-733)
 * @author EdgeOS Core Architecture Team
 *
 * @details High-throughput, cache-aligned spatial matrix and sliding window polarization
 *          feature extractor for up to 4096 battery cells. Extracts 1st/2nd order differentials
 *          (dV/dt, dT/dt, d2V/dt2), dynamic polarization resistance (Rp), spatial 3D thermal
 *          Laplacian gradients, and predicts thermal runaway diffusion vectors within <10ms.
 */

#ifndef BATTERY_SPATIAL_MATRIX_H
#define BATTERY_SPATIAL_MATRIX_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BATTERY_SPATIAL_MAX_CELLS        4096
#define BATTERY_SPATIAL_WINDOW_LEN       16
#define BATTERY_TOP_OUTLIERS_MAX         8
#define BATTERY_DIFFUSION_MAX_HOTSPOTS   16

/**
 * @brief 5-Level Battery Thermal Runaway Evolution Stage Classification
 */
typedef enum {
    BATTERY_TR_STAGE_NORMAL = 0,         /**< Normal operating conditions (<45°C, dT/dt < 0.1°C/s) */
    BATTERY_TR_STAGE_ATTENTION = 1,      /**< Attention: mild abnormal heating (45~55°C, dT/dt >= 0.2°C/s) */
    BATTERY_TR_STAGE_SELF_HEATING = 2,   /**< Warning: SEI decomposition self-heating (55~70°C, dT/dt >= 0.5°C/s) */
    BATTERY_TR_STAGE_ACCELERATION = 3,   /**< Critical: separator breakdown acceleration (70~90°C, dT/dt >= 1.0°C/s) */
    BATTERY_TR_STAGE_EXPLOSIVE = 4       /**< Immediate Danger: explosive thermal runaway (>=90°C, dT/dt >= 2.0°C/s) */
} battery_tr_stage_t;

/**
 * @brief 3D Spatial Topology Coordinate of a Cell
 */
typedef struct {
    uint16_t rack_id;
    uint16_t module_id;
    uint16_t cell_id_in_module;
    float pos_x; /**< Cartesian X in meters */
    float pos_y; /**< Cartesian Y in meters */
    float pos_z; /**< Cartesian Z in meters */
} battery_cell_coord_t;

/**
 * @brief Per-cell Sliding Window Time-Series Ringbuffer
 */
typedef struct {
    float v_history[BATTERY_SPATIAL_WINDOW_LEN];
    float t_history[BATTERY_SPATIAL_WINDOW_LEN];
    float i_history[BATTERY_SPATIAL_WINDOW_LEN];
    uint64_t ts_history[BATTERY_SPATIAL_WINDOW_LEN]; /**< Timestamp in milliseconds */
    uint8_t head;
    uint8_t count;
} battery_cell_window_t;

/**
 * @brief Extracted Polarization & Thermal Features for a Single Cell
 */
typedef struct {
    float voltage;              /**< Latest cell voltage (V) */
    float temperature;          /**< Latest cell temperature (°C) */
    float current;              /**< Latest cell current (A) */
    float soc;                  /**< Estimated state of charge (0.0~1.0) */
    float dV_dt;                /**< 1st order voltage rate-of-change (V/s) */
    float dT_dt;                /**< 1st order temperature rise rate (°C/s) */
    float d2V_dt2;              /**< 2nd order voltage curvature (V/s^2) */
    float overpotential;        /**< Polarization overpotential eta (V) */
    float polarization_res;     /**< Dynamic polarization resistance Rp (Ohm) */
    float spatial_temp_grad;    /**< Maximum spatial temperature gradient with adjacent neighbors (°C/m) */
    battery_tr_stage_t tr_stage;/**< Current cell thermal runaway risk stage */
} battery_cell_features_t;

/**
 * @brief Thermal Runaway Hotspot & Spatial Diffusion Vector
 */
typedef struct {
    uint32_t cell_index;
    battery_cell_coord_t coord;
    float temperature;
    float dT_dt;
    float spatial_grad;
    float diffusion_vec_x;             /**< Thermal propagation vector X component */
    float diffusion_vec_y;             /**< Thermal propagation vector Y component */
    float diffusion_vec_z;             /**< Thermal propagation vector Z component */
    float estimated_propagation_speed; /**< Estimated propagation velocity (m/s) */
    battery_tr_stage_t stage;
} battery_diffusion_hotspot_t;

/**
 * @brief Outlier Item Tracking
 */
typedef struct {
    uint32_t cell_index;
    float value;
    float z_score;
} battery_outlier_t;

/**
 * @brief Aggregated Spatial Frame Computation Result
 */
typedef struct {
    uint32_t total_cells;
    uint64_t timestamp_ms;

    // Voltage statistics
    float v_min;
    float v_max;
    float v_mean;
    float v_std;
    float v_median;
    uint32_t v_min_cell_idx;
    uint32_t v_max_cell_idx;

    // Temperature statistics
    float t_min;
    float t_max;
    float t_mean;
    float t_std;
    float t_median;
    uint32_t t_min_cell_idx;
    uint32_t t_max_cell_idx;

    // Dynamic rate extremes
    float max_dT_dt;
    uint32_t max_dT_dt_cell_idx;
    float max_d2V_dt2;
    uint32_t max_d2V_dt2_cell_idx;

    // Pack-level overall TR stage (highest stage among all cells)
    battery_tr_stage_t overall_tr_stage;

    // Top statistical outliers
    uint32_t num_voltage_outliers;
    battery_outlier_t voltage_outliers[BATTERY_TOP_OUTLIERS_MAX];
    uint32_t num_temp_outliers;
    battery_outlier_t temp_outliers[BATTERY_TOP_OUTLIERS_MAX];

    // Thermal diffusion hotspots
    uint32_t num_hotspots;
    battery_diffusion_hotspot_t hotspots[BATTERY_DIFFUSION_MAX_HOTSPOTS];

    // Compute execution benchmark (microseconds)
    uint64_t compute_duration_us;
} battery_spatial_frame_result_t;

/**
 * @brief Configuration Parameters for Spatial Matrix Extractor
 */
typedef struct {
    uint32_t num_cells;                 /**< Total cells (1..4096) */
    uint16_t num_racks;                 /**< Number of battery racks */
    uint16_t num_modules_per_rack;      /**< Modules per rack */
    uint16_t num_cells_per_module;      /**< Cells per module */
    float nominal_capacity_ah;          /**< Nominal cell capacity in Ah (e.g. 280.0) */
    float internal_r0_mohm;             /**< Baseline ohmic internal resistance in mOhm (e.g. 0.25) */

    // 3D Grid spacing in meters
    float cell_dx;                      /**< Pitch between adjacent cells in X (m) */
    float cell_dy;                      /**< Pitch between adjacent modules in Y (m) */
    float cell_dz;                      /**< Pitch between adjacent racks in Z (m) */

    // Thermal Runaway classification thresholds
    float temp_attention_thresh;        /**< Default 45.0 °C */
    float temp_self_heating_thresh;     /**< Default 55.0 °C */
    float temp_acceleration_thresh;     /**< Default 70.0 °C */
    float temp_explosive_thresh;        /**< Default 90.0 °C */

    float dtdt_attention_thresh;        /**< Default 0.2 °C/s */
    float dtdt_self_heating_thresh;     /**< Default 0.5 °C/s */
    float dtdt_acceleration_thresh;     /**< Default 1.0 °C/s */
    float dtdt_explosive_thresh;        /**< Default 2.0 °C/s */

    float z_score_outlier_thresh;       /**< Default 3.0 sigma */
} battery_spatial_matrix_config_t;

/**
 * @brief Full 4096-Node Spatial Matrix & Ringbuffer State
 * @warning This structure is ~2.3MB in size. NEVER allocate on thread stack.
 *          Always use battery_spatial_matrix_create() or heap allocation.
 */
typedef struct {
    battery_spatial_matrix_config_t config;
    battery_cell_coord_t coords[BATTERY_SPATIAL_MAX_CELLS];
    battery_cell_window_t windows[BATTERY_SPATIAL_MAX_CELLS];
    battery_cell_features_t features[BATTERY_SPATIAL_MAX_CELLS];

    // Flat contiguous cache-aligned working arrays for SIMD vectorization
    float raw_v[BATTERY_SPATIAL_MAX_CELLS];
    float raw_t[BATTERY_SPATIAL_MAX_CELLS];
    float raw_i[BATTERY_SPATIAL_MAX_CELLS];

    // Precalculated 6-connectivity 3D neighbor table (-1 for boundary)
    // 0: -X, 1: +X, 2: -Y, 3: +Y, 4: -Z, 5: +Z
    int32_t neighbor_indices[BATTERY_SPATIAL_MAX_CELLS][6];
} battery_spatial_matrix_t;

/**
 * @brief Safely allocate on heap and initialize 4096-Node Spatial Matrix context
 * @note Allocates ~2.3MB on heap. Safe for embedded RTOS threads.
 * @param config Pointer to configuration struct
 * @return Pointer to initialized context on success, NULL on failure
 */
battery_spatial_matrix_t *battery_spatial_matrix_create(const battery_spatial_matrix_config_t *config);

/**
 * @brief Free memory allocated for 4096-Node Spatial Matrix context
 * @param ctx Pointer to context created via battery_spatial_matrix_create
 */
void battery_spatial_matrix_destroy(battery_spatial_matrix_t *ctx);

/**
 * @brief Initialize existing 4096-Node Spatial Matrix context and calculate 3D topology
 * @warning Ensure ctx is allocated on the heap, not on the function stack.
 * @param ctx Pointer to battery_spatial_matrix_t context
 * @param config Pointer to configuration struct
 * @return 0 on success, negative error code on failure
 */
int battery_spatial_matrix_init(battery_spatial_matrix_t *ctx, const battery_spatial_matrix_config_t *config);

/**
 * @brief Override or set custom 3D coordinates for a cell
 * @param ctx Pointer to initialized context
 * @param cell_idx Cell index (0..num_cells-1)
 * @param coord Pointer to custom coordinate struct
 * @return 0 on success, negative error code on failure
 */
int battery_spatial_matrix_set_coord(battery_spatial_matrix_t *ctx, uint32_t cell_idx, const battery_cell_coord_t *coord);

/**
 * @brief Process a raw frame of sensor readings across all cells
 * @param ctx Pointer to initialized context
 * @param voltages Array of cell voltages (size >= num_cells)
 * @param temperatures Array of cell temperatures in °C (size >= num_cells)
 * @param currents Array of cell currents in A (size >= num_cells or NULL for global current)
 * @param num_cells Number of cells in frame (must match configured num_cells)
 * @param timestamp_ms Frame timestamp in milliseconds
 * @param out_result Output aggregated frame analytics result
 * @return 0 on success, negative error code on failure
 */
int battery_spatial_matrix_process_frame(battery_spatial_matrix_t *ctx,
                                        const float *voltages,
                                        const float *temperatures,
                                        const float *currents,
                                        uint32_t num_cells,
                                        uint64_t timestamp_ms,
                                        battery_spatial_frame_result_t *out_result);

/**
 * @brief Retrieve extracted polarization & thermal features for a single cell
 * @param ctx Pointer to initialized context
 * @param cell_idx Cell index (0..num_cells-1)
 * @param out_features Output features struct
 * @return 0 on success, negative error code on failure
 */
int battery_spatial_matrix_get_cell_features(const battery_spatial_matrix_t *ctx, uint32_t cell_idx, battery_cell_features_t *out_features);

/**
 * @brief Reset sliding windows and state history
 * @param ctx Pointer to initialized context
 * @return 0 on success, negative error code on failure
 */
int battery_spatial_matrix_reset(battery_spatial_matrix_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* BATTERY_SPATIAL_MATRIX_H */
