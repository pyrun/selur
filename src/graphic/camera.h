#ifndef CAMERA_INCLUDED_H
#define CAMERA_INCLUDED_H 1

#define GLM_ENABLE_EXPERIMENTAL

#include <stdio.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

struct Camera {
public:
	Camera(const glm::vec3& pos, float fov, float aspect, float zNear, float zFar, float ortho_size = -50);

	void MoveForward(float amt);
	void MoveForwardCross( float amt);
	void MoveUp( float amt);
	void MoveRight(float amt);

	void verticalAngle (float angle, float push = false);
	void horizontalAngle (float angle, bool push = false);
	void resize( float aspect);
	void zoom( float change = 0.0f);

    glm::mat4 getViewProjection() const{ return p_projection * glm::lookAt(p_position, p_position + p_direction, p_up); }
    glm::mat4 getView() const{ return glm::lookAt(p_position, p_position + p_direction, p_up); }
    glm::mat4 getProjection() const{ return p_projection; }
	glm::mat4 getProjectionOrtho() const{ return p_projection_ortho; }

	inline glm::vec3 getPos() { return p_position; }
	inline glm::vec3 getForward() { return p_direction; }
	inline glm::vec3 getUp() { return p_up; }

	void setPos( glm::vec3 position) { p_position = position; }
	void addPos( glm::vec3 position) { p_position += position; }

	inline glm::mat4 getViewWithoutUp() { return glm::lookAt(p_position, p_position + glm::vec3( p_direction.x, 0.f, p_direction.z ), glm::vec3( 0, 1.0f, 0)); }

	void setDirection( glm::vec3 direction)  { p_direction = direction; }
protected:
    void calcOrientation();
private:
	glm::mat4 p_projection;
	glm::mat4 p_projection_ortho;
	glm::vec3 p_position;
	glm::vec3 p_direction;
	glm::vec3 p_up;

	float p_fov;
    float p_horizontalAngle;
    float p_verticalAngle;

    float p_aspect;
    float p_zNear;
    float p_zFar;
};

#endif
