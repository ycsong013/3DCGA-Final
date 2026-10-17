#include "bezier.h"
// Suppress warnings in third-party code.
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <tinyobjloader/tiny_obj_loader.h>
DISABLE_WARNINGS_POP()
#include <algorithm>
#include <cassert>
#include <exception>
#include <iostream>
#include <numeric>
#include <span>
#include <stack>
#include <string>
#include <tuple>
#include <map>

glm::vec3 bezier1(const glm::vec3& l0, const glm::vec3& l1, float t) {
	return (1 - t) * l0 + t * l1;
}
glm::vec3 bezier1derivative(const glm::vec3& l0, const glm::vec3& l1, float t) {
	return l1 - l0;
}

glm::vec3 bezier2(const glm::vec3& q0, const glm::vec3& q1, const glm::vec3& q2, float t) {
	return (1 - t) * (1 - t) * q0 
		+ 2 * (1 - t) * t * q1 
		+ t * t * q2;
}
glm::vec3 bezier2derivative(const glm::vec3& q0, const glm::vec3& q1, const glm::vec3& q2, float t) {
	return (2 * t - 2) * q0 
		+ (-4 * t + 2) * q1 
		+ 2 * t * q2;
}

glm::vec3 bezier3(const glm::vec3& c0, const glm::vec3& c1, const glm::vec3& c2, const glm::vec3& c3, float t) {
	return (1 - t) * (1 - t) * (1 - t) * c0
		+ 3 * (1 - t) * (1 - t) * t * c1
		+ 3 * (1 - t) * t * t * c2
		+ t * t * t * c3;
}
glm::vec3 bezier3derivative(const glm::vec3& c0, const glm::vec3& c1, const glm::vec3& c2, const glm::vec3& c3, float t) {
	return (-3 * t * t + 6 * t - 3) * c0
		+ (9 * t * t - 12 * t + 3) * c1
		+ (-9 * t * t + 6 * t) * c2
		+ 3 * t * t * c3;
}