#ifndef DIFF_DRIVE_H
#define DIFF_DRIVE_H

/* Dong hoc robot 2 banh vi sai - Differential Drive Robot */

typedef struct
{
    float radius;
    float track;
    float max_rpm;
} DiffDrive;

void DiffDrive_Init(DiffDrive *d, float radius, float track, float max_rpm);
void DiffDrive_ToWheels(const DiffDrive *d, float v, float w, float *left_rpm, float *right_rpm);
void DiffDrive_ToBody(const DiffDrive *d, float left_rpm, float right_rpm, float *v, float *w);

#endif
