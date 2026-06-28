#ifndef FOC_H
#define FOC_H

#include <stdint.h>
#include <math.h>

/* ============================================================================
 * MACRO TOÁN HỌC & CẤU HÌNH HỆ THỐNG
 * ============================================================================*/
#define TWO_PI          6.28318530718f
#define SQRT3_DIV_2     0.86602540378f
#define ONE_BY_SQRT3    0.57735026919f

#define POLE_PAIRS      7.0f
#define F_PWM           23821.0f
#define T_SAMPLE        (1.0f / F_PWM)
#define PWM_PERIOD_ARR  2099.0f

/* ============================================================================
 * ĐỊNH NGHĨA NỐT NHẠC
 * ============================================================================*/
#ifndef NOTE_REST
#define NOTE_REST 0.0f
#endif
#ifndef NOTE_C6
#define NOTE_C6 1046.50f
#endif
#ifndef NOTE_E6
#define NOTE_E6 1318.51f
#endif
#ifndef NOTE_G6
#define NOTE_G6 1567.98f
#endif

typedef struct {
    float frequency;
    float duration;
} MusicNote;

typedef struct {
    float Kp;
    float Ki;
    float error_sum;
    float out_max;
} PI_Controller;

typedef enum {
    STATE_MUSIC = 0,
    STATE_ALIGN,
    STATE_OPEN_LOOP,
    STATE_CLOSED_LOOP // Dành cho tương lai
} SystemState;

/* ============================================================================
 * CẤU TRÚC ĐỐI TƯỢNG ĐỘNG CƠ (OOP in C)
 * ============================================================================*/
typedef struct {
    // 1. Trạng thái hệ thống
    SystemState state;

    // 2. Thông số vật lý & Cấu hình
    float Vbus;
    float Vq_test;       // Điện áp bơm vào Vq khi chạy Open-Loop
    float align_volts;   // Điện áp dùng để Align

    // 3. Quản lý tốc độ (Open Loop)
    float target_RPM;
    float current_RPM;
    uint16_t theta_16bit;
    int is_CW;

    // 4. Các biến thời gian (Timer)
    float align_timer;
    float note_timer;
    uint16_t current_note_index;
    uint16_t tone_phase_16bit;

    // 5. Đo lường dòng điện
    float Ia, Ib, Ic;

    // 6. Điện áp và Duty Cycle đầu ra
    float Valpha, Vbeta;
    float dutyA, dutyB, dutyC;

    // 7. Bộ PI cho Closed-Loop (Tương lai)
    PI_Controller pi_d;
    PI_Controller pi_q;

} FOC_Motor;

/* ============================================================================
 * PROTOTYPES GIAO TIẾP (API)
 * ============================================================================*/
void FOC_Init(FOC_Motor *motor);
void FOC_Routine(FOC_Motor *motor);
void FOC_SetPWM_Hardware(float dutyA, float dutyB, float dutyC);

#endif // FOC_H
