#include "diff_drive.h"

#include <math.h>

#define PI 3.14159265358979323846f

void DiffDrive_Init(DiffDrive *d, float radius, float track, float max_rpm)
{
    d->radius = radius;
    d->track = track;
    d->max_rpm = max_rpm;
}

/* Dong hoc nghich: Chuyen doi v, w thanh RPM */
void DiffDrive_ToWheels(const DiffDrive *d, float v, float w, float *left_rpm, float *right_rpm)
{
    // He so vong quay
    float k = 60.0f / (2.0f * PI * d->radius);

    // Tinh toan RPM cho 2 banh xe
    *left_rpm = (v - 0.5f * d->track * w) * k;
    *right_rpm = (v + 0.5f * d->track * w) * k;

    float left_abs = fabsf(*left_rpm);
    float right_abs = fabsf(*right_rpm);

    // Kiem tra va dieu chinh neu vuot qua gioi han max_rpm
    float peak = left_abs > right_abs ? left_abs : right_abs;
    if (peak > d->max_rpm) {
        float scale = d->max_rpm / peak;
        *left_rpm *= scale;
        *right_rpm *= scale;
    }
}

/* Dong hoc thuan: Chuyen doi RPM thanh v, w */
void DiffDrive_ToBody(const DiffDrive *d, float left_rpm, float right_rpm, float *v, float *w)
{
    // He so vong quay
    float k = 2.0f * PI * d->radius / 60.0f;

    // Tinh toan van toc dai va van toc goc
    *v = 0.5f * (left_rpm + right_rpm) * k;
    *w = (right_rpm - left_rpm) * k / d->track;
}
