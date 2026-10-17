#pragma once

#include <framework/disable_all_warnings.h>
#include <framework/mesh.h>
#include <framework/shader.h>
DISABLE_WARNINGS_PUSH()
#include <glm/vec3.hpp>
DISABLE_WARNINGS_POP()

#include <exception>
#include <filesystem>
#include <framework/opengl_includes.h>
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>

// Linear Bezier: 
// B(t) = (1-t) * L0 + t * L1
// B'(t) = L1 - L0
glm::vec3 bezier1(const glm::vec3& l0, const glm::vec3& l1, float t);
glm::vec3 bezier1derivative(const glm::vec3& l0, const glm::vec3& l1, float t);

// Quadratic Bezier: 
// B(t) = (1-t)^2 * Q0 + 2 * (1-t)*t * Q1 + t^2 * Q2
// B'(t) = (2t-2) * Q0 + (-4t+2) * Q1 + 2t * Q2
glm::vec3 bezier2(const glm::vec3& q0, const glm::vec3& q1, const glm::vec3& q2, float t);
glm::vec3 bezier2derivative(const glm::vec3& q0, const glm::vec3& q1, const glm::vec3& q2, float t);

// Cubic Bezier:
// B(t) = (1-t)^3 * C0 + 3 * (1-t)^2*t * C1 + 3 * (1-t)*t^2 * C2 + t^3 * C3
// B'(t) = (-3t^2+6t-3) * C0 + (9t^2-12t+3) * C1 + (-9t^2+6t) * C2 + 3t^2 * C3
glm::vec3 bezier3(const glm::vec3& c0, const glm::vec3& c1, const glm::vec3& c2, const glm::vec3& c3, float t);
glm::vec3 bezier3derivative(const glm::vec3& c0, const glm::vec3& c1, const glm::vec3& c2, const glm::vec3& c3, float t);

struct bezierPath {
	// The entire path are composed of 3 Bezier Path segments:
	// Linear --> Quadratic --> Cubic, then back to the starting point
	// Right now we have L0(C3), L1(Q0), Q1, Q2(C0), C1, C2, 6 verteces in total
	glm::vec3 L0, L1;           // L1 = Q0;
	glm::vec3 Q0, Q1, Q2;       // Q2 = C0
	glm::vec3 C0, C1, C2, C3;   // C3 = L0

	// To ensure the whole loop is smooth, the connecting joints must have the same derivative, namely
	// L1 - L0 = 2 * (Q1 - Q0)
	// 2 * (Q2 - Q1) = 3 * (C1 - C0)
	// 3 * (C3 - C2) = L1 - L0,
	// which means we have 3 free verteces: L0, L1, Q2
	void build(const glm::vec3& l0, const glm::vec3& l1, const glm::vec3& q2) {
		C3 = L0 = l0;
		L1 = l1;

		Q0 = L1;
		Q1 = (L1 - L0) / 2.0f + Q0;
		Q2 = q2;

		C0 = Q2;
		C1 = 2.0f * (Q2 - Q1) / 3.0f + C0;
		C2 = C3 - (L1 - L0) / 3.0f;
	}

	void calcPosAndDir(float posOnPath, glm::vec3& pos, glm::vec3& dir) {
		// posOnPath in [0, 3)
		// For example, posOnPath = 1,3 means the boat is right now on the 2nd segment (the quadratic bezier), and t = 0,3

		int segment = static_cast<int>(posOnPath);
		float t = posOnPath - segment;

		switch (segment) {
		case 0:
			pos = bezier1(L0, L1, t);
			dir = glm::normalize(bezier1derivative(L0, L1, t));
			break;
		case 1:
			pos = bezier2(Q0, Q1, Q2, t);
			dir = glm::normalize(bezier2derivative(Q0, Q1, Q2, t));
			break;
		case 2:
			pos = bezier3(C0, C1, C2, C3, t);
			dir = glm::normalize(bezier3derivative(C0, C1, C2, C3, t));
			break;
		default:
			break;
		}
	}
};