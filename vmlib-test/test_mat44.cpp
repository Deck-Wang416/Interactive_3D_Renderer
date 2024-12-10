#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include "mat44.hpp"

TEST_CASE("Matrix Multiplication") {
    Mat44f m1 = kIdentity44f;
    Mat44f m2 = kIdentity44f;
    Mat44f result = m1 * m2;
    REQUIRE(result == kIdentity44f);
}

TEST_CASE("Matrix-Vector Multiplication") {
    Mat44f m = kIdentity44f;
    Vec4f v = {1.0f, 2.0f, 3.0f, 1.0f};
    Vec4f result = m * v;
    REQUIRE(result == v);
}

TEST_CASE("Rotation Matrices") {
    float angle = M_PI / 2.0f;
    Mat44f rx = make_rotation_x(angle);
    Mat44f ry = make_rotation_y(angle);
    Mat44f rz = make_rotation_z(angle);
    REQUIRE(rx(1, 2) == Approx(-1.0f).margin(0.01));
    REQUIRE(ry(0, 2) == Approx(1.0f).margin(0.01));
    REQUIRE(rz(0, 1) == Approx(-1.0f).margin(0.01));
}

TEST_CASE("Translation Matrix") {
    Vec3f translation = {1.0f, 2.0f, 3.0f};
    Mat44f t = make_translation(translation);
    REQUIRE(t(0, 3) == Approx(1.0f));
    REQUIRE(t(1, 3) == Approx(2.0f));
    REQUIRE(t(2, 3) == Approx(3.0f));
}

TEST_CASE("Perspective Projection Matrix") {
    float fov = M_PI / 2.0f;
    float aspect = 16.0f / 9.0f;
    float near = 0.1f;
    float far = 100.0f;
    Mat44f p = make_perspective_projection(fov, aspect, near, far);
    REQUIRE(p(0, 0) == Approx(1.0f / (aspect * std::tan(fov / 2.0f))).margin(0.01));
    REQUIRE(p(3, 2) == Approx(-1.0f).margin(0.01));
}
