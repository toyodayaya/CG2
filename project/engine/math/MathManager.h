#pragma once

// 構造体の宣言
struct Vector2
{
	float x, y;
};

struct Vector3
{
	float x, y, z;
};

struct Vector4
{
	float x, y, z, w;
};

struct Matrix3x3
{
	float m[3][3];
};

struct Matrix4x4
{
	float m[4][4];
};

struct Transform
{
	Vector3 scale;
	Vector3 rotate;
	Vector3 translate;
};

struct AABB
{
	Vector3 min;
	Vector3 max;
};

namespace MathManager
{
	// 単位行列の作成
	Matrix4x4 MakeIdentity4x4();

	// 回転行列
	Matrix4x4 MakeRotateXMatrix(float radian);

	Matrix4x4 MakeRotateYMatrix(float radian);

	Matrix4x4 MakeRotateZMatrix(float radian);

	Matrix4x4 MakeScaleMatrix(const Vector3& scale);

	Matrix4x4 MakeTranslateMatrix(const Vector3& translate);

	// 行列の積
	Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2);

	// アフィン変換行列
	Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate);

	// 透視投影行列
	Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip);

	// 正射影行列
	Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip);

	// 逆行列
	Matrix4x4 Inverse(const Matrix4x4& m);

	// 5.転置行列
	Matrix4x4 Transpose(const Matrix4x4& m);

	// Vector3の足し算
	Vector3 Vector3Add(const Vector3& v1, const Vector3& v2);

}