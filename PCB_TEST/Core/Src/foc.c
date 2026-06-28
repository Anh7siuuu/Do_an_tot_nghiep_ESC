#include "foc.h"

// Macro nội bộ
#define MAX3(a, b, c)  ((a > b) ? ((a > c) ? a : c) : ((b > c) ? b : c))
#define MIN3(a, b, c)  ((a < b) ? ((a < c) ? a : c) : ((b < c) ? b : c))
#define LUT_SIZE 1024
// Macro hỗ trợ viết nốt nhạc cho gọn
#define N(note, dur) {note, dur - 0.04f}, {NOTE_REST, 0.04f}
#define R(dur)       {NOTE_REST, dur}

// Biến toàn cục nội bộ
static float sin_lut[LUT_SIZE];
static float cos_lut[LUT_SIZE];
static int is_music_playing = 1;

MusicNote startup_melody[] = {
    R(0.1f),
    N(NOTE_C6, 0.2f),
	R(0.3f),
    N(NOTE_E6, 0.2f),
	R(0.3f),
    N(NOTE_G6, 0.6f),
    R(0.1f)
};
const uint16_t melody_length = sizeof(startup_melody) / sizeof(MusicNote);
/* ============================================================================
 * HÀM TOÁN HỌC CƠ BẢN
 * ============================================================================*/
static void LUT_SinCos_16bit(uint16_t angle, float *s, float *c) {
    int index = angle >> 6;
    int next_index = (index + 1) & 1023;
    float fraction = (angle & 0x3F) * 0.015625f;

    *s = sin_lut[index] + fraction * (sin_lut[next_index] - sin_lut[index]);
    *c = cos_lut[index] + fraction * (cos_lut[next_index] - cos_lut[index]);
}

static void SVPWM_Compute(FOC_Motor *motor, float Valpha, float Vbeta) {
    float Va = Valpha;
    float Vb = -0.5f * Valpha + SQRT3_DIV_2 * Vbeta;
    float Vc = -0.5f * Valpha - SQRT3_DIV_2 * Vbeta;

    float Vmax = MAX3(Va, Vb, Vc);
    float Vmin = MIN3(Va, Vb, Vc);
    float V_shift = -(Vmax + Vmin) / 2.0f;

    float Va_svpwm = Va + V_shift;
    float Vb_svpwm = Vb + V_shift;
    float Vc_svpwm = Vc + V_shift;

    motor->dutyA = (Va_svpwm / motor->Vbus) + 0.5f;
    motor->dutyB = (Vb_svpwm / motor->Vbus) + 0.5f;
    motor->dutyC = (Vc_svpwm / motor->Vbus) + 0.5f;

    // Cắt ngọn an toàn
    if (motor->dutyA > 1.0f) motor->dutyA = 1.0f; else if (motor->dutyA < 0.0f) motor->dutyA = 0.0f;
    if (motor->dutyB > 1.0f) motor->dutyB = 1.0f; else if (motor->dutyB < 0.0f) motor->dutyB = 0.0f;
    if (motor->dutyC > 1.0f) motor->dutyC = 1.0f; else if (motor->dutyC < 0.0f) motor->dutyC = 0.0f;
}

/* ============================================================================
 * LOGIC ĐIỀU KHIỂN FOC
 * ============================================================================*/
void FOC_Init(FOC_Motor *motor) {
    // Khởi tạo bảng LUT
    for (int i = 0; i < LUT_SIZE; i++) {
        float angle = (float)i * (TWO_PI / (float)LUT_SIZE);
        sin_lut[i] = sinf(angle);
        cos_lut[i] = cosf(angle);
    }

    // Khởi tạo các giá trị mặc định cho Motor
    motor->state = STATE_MUSIC;
    motor->Vbus = 14.8f;
    motor->Vq_test = 1.5f;
    motor->align_volts = 1.0f;
    motor->target_RPM = 1000.0f;
    motor->current_RPM = 0.0f;
    motor->theta_16bit = 0;
    motor->is_CW = 1;

    motor->align_timer = 0.0f;
    motor->note_timer = 0.0f;
    motor->current_note_index = 0;
    motor->tone_phase_16bit = 0;

    // PI Defaults
    motor->pi_d.Kp = 0.5f; motor->pi_d.Ki = 0.01f; motor->pi_d.out_max = 12.0f;
    motor->pi_q.Kp = 0.5f; motor->pi_q.Ki = 0.01f; motor->pi_q.out_max = 12.0f;
}

static void FOC_PlayMusic(FOC_Motor *motor) {
    if (!is_music_playing || melody_length == 0) {
        motor->state = STATE_ALIGN;
        return;
    }

    motor->note_timer += T_SAMPLE;

    if (motor->note_timer >= startup_melody[motor->current_note_index].duration) {
        motor->note_timer = 0.0f;
        motor->current_note_index++;

        if (motor->current_note_index >= melody_length) {
            motor->state = STATE_ALIGN;
            motor->align_timer = 0.0f;
            FOC_SetPWM_Hardware(0.5f, 0.5f, 0.5f);
            return;
        }
    }

    float current_freq = startup_melody[motor->current_note_index].frequency;

    if (current_freq > 0.0f) {
        uint16_t phase_step = (uint16_t)((current_freq * T_SAMPLE) * 65536.0f);
        motor->tone_phase_16bit += phase_step;

        float tone_sin, tone_cos;
        LUT_SinCos_16bit(motor->tone_phase_16bit, &tone_sin, &tone_cos);

        motor->Valpha = 2.0f * tone_sin; // 3.5f MUSIC_VOLUME_VOLTAGE
        motor->Vbeta  = 0.0f;

        SVPWM_Compute(motor, motor->Valpha, motor->Vbeta);
        FOC_SetPWM_Hardware(motor->dutyA, motor->dutyB, motor->dutyC);
    } else {
        FOC_SetPWM_Hardware(0.5f, 0.5f, 0.5f);
    }
}

static void FOC_Align(FOC_Motor *motor) {
    motor->align_timer += T_SAMPLE;

    // ----------------------------------------------------------------
    // GIAI ĐOẠN 1: QUÉT CHẬM 2 VÒNG ĐIỆN (0.0s -> 0.8s)
    // ----------------------------------------------------------------
    if (motor->align_timer < 0.8f) {
        float progress = motor->align_timer / 0.8f;
        uint16_t align_angle_16bit = (uint16_t)(progress * 65535.0f * 2.0f);

        float align_sin, align_cos;
        LUT_SinCos_16bit(align_angle_16bit, &align_sin, &align_cos);

        motor->Valpha = motor->align_volts * align_cos;
        motor->Vbeta  = motor->align_volts * align_sin;
    }
    // ----------------------------------------------------------------
    // GIAI ĐOẠN 2: KHÓA CHẶT (0.8s -> 1.2s)
    // ----------------------------------------------------------------
    else if (motor->align_timer < 1.2f) {
        float align_sin, align_cos;
        LUT_SinCos_16bit(0, &align_sin, &align_cos); // Ép chết ở 0 độ

        motor->Valpha = motor->align_volts * align_cos;
        motor->Vbeta  = motor->align_volts * align_sin;
    }
    // ----------------------------------------------------------------
    // GIAI ĐOẠN 3: XUẤT PHÁT
    // ----------------------------------------------------------------
    else {
        motor->state = STATE_OPEN_LOOP;
        motor->theta_16bit = 0;

        // Trả về 0.0f êm ái như code cũ vì OpenLoop đã có Kick-start lo
        motor->current_RPM = 0.0f;
        return;
    }

    SVPWM_Compute(motor, motor->Valpha, motor->Vbeta);
    FOC_SetPWM_Hardware(motor->dutyA, motor->dutyB, motor->dutyC);
}
static void FOC_OpenLoop(FOC_Motor *motor) {
	float rpm_increment;
	float startup_boost_V = 0.0f;

	// 1. CHỈNH LOGIC GIA TỐC: HẠ XUỐNG ĐỂ ĐỘNG CƠ "THỞ"
	if (motor->current_RPM < 1500.0f) {
		rpm_increment = 900.0f * T_SAMPLE; // Hạ gia tốc xuống mức "siêu rùa"
		startup_boost_V = 3.0f;           // Bơm mạnh để thắng ma sát tĩnh
	} else {
		rpm_increment = 1000.0f * T_SAMPLE;
		startup_boost_V = 0.0f;
	}

	if (motor->current_RPM < 0.0f) motor->current_RPM = 0.0f;
    // Thực thi Tăng/Giảm tốc độ bám theo safe_target
	if (motor->current_RPM < motor->target_RPM) {
		motor->current_RPM += rpm_increment;
		if (motor->current_RPM > motor->target_RPM) motor->current_RPM = motor->target_RPM;
	} else if (motor->current_RPM > motor->target_RPM) {
		motor->current_RPM -= rpm_increment;
		if (motor->current_RPM < motor->target_RPM) motor->current_RPM = motor->target_RPM;
	}

    // Quy đổi tốc độ RPM ra bước nhảy DDS
    float speed_target = motor->current_RPM * (TWO_PI / 60.0f) * POLE_PAIRS * T_SAMPLE;
    uint16_t speed_step_16bit = (uint16_t)(speed_target * 10430.378f);
    // ----------------------------------------------------------------
    // 2. TÍNH TOÁN ĐIỆN ÁP (V/f)
    // ----------------------------------------------------------------
    float Vq_boost = motor->current_RPM * 0.0012f;
    float Vq_actual = motor->Vq_test + Vq_boost + startup_boost_V;

    // Cắt ngọn để bảo vệ mạch
	if (Vq_actual > motor->Vbus) Vq_actual = motor->Vbus;

	// Nếu target = 0, ép điện áp về 0 hẳn để nó không bị rung
	if (motor->target_RPM < 100.0f) Vq_actual = 0.0f;
    // ----------------------------------------------------------------
    // 3. CẬP NHẬT GÓC ĐIỆN VÀ INVERSE PARK
    // ----------------------------------------------------------------
    if (motor->is_CW) {
        motor->theta_16bit -= speed_step_16bit;
    } else {
        motor->theta_16bit += speed_step_16bit;
    }

    float sin_t, cos_t;
    LUT_SinCos_16bit(motor->theta_16bit, &sin_t, &cos_t);

    float Vd = 0.0f;
    float Vq = Vq_actual;
    if (motor->is_CW){
        Vq = -Vq_actual;
    }

    motor->Valpha = Vd * cos_t - Vq * sin_t;
    motor->Vbeta  = Vd * sin_t + Vq * cos_t;

    // ----------------------------------------------------------------
    // 4. SVPWM & GHI VÀO THANH GHI TIMER
    // ----------------------------------------------------------------
    SVPWM_Compute(motor, motor->Valpha, motor->Vbeta);
    FOC_SetPWM_Hardware(motor->dutyA, motor->dutyB, motor->dutyC);
}
void FOC_Routine(FOC_Motor *motor) {
    switch (motor->state) {
        case STATE_MUSIC:
            FOC_PlayMusic(motor);
            break;
        case STATE_ALIGN:
            FOC_Align(motor);
            break;
        case STATE_OPEN_LOOP:
            FOC_OpenLoop(motor);
            break;
        case STATE_CLOSED_LOOP:
            break;
    }
}
