#include "mesh_renderer.hpp"

struct Sprite {
	Vec3f position;
	Vec3f velocity;
	float lifespan;
};

std::vector<Sprite> sprites;
GLuint texture, VBO, VAO;
int maxSprites = 6000;


void loadTexture() { 
	texture = load_texture_2d("assets/fire.jpg"); 
	glGetError();
}

void setupSpriteBuffers() {
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, maxSprites * 3 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0); 
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

std::mt19937 createRandomEngine() {
	std::random_device rd;
	return std::mt19937(rd());
}

std::uniform_real_distribution<> createUniformDistribution(double min, double max) {
	return std::uniform_real_distribution<>(min, max);
}

float generateRandomValue(std::mt19937& gen, std::uniform_real_distribution<>& dis) {
	return dis(gen);
}

Vec3f computeDirection(float phi, float theta) {
	Vec3f randomDirection;
	randomDirection.x = sin(phi) * cos(theta);
	randomDirection.y = sin(phi) * sin(theta);
	randomDirection.z = cos(phi);
	return randomDirection;
}

Vec3f randomConicalDirection(Vec3f direction) {
	// Initialize random engine and distribution
	static std::mt19937 gen = createRandomEngine();
	static std::uniform_real_distribution<> dis(0, 1);

	// Generate random spherical coordinates
	float theta = 2 * kPi_ * generateRandomValue(gen, dis);
	float phi = acos(1 - generateRandomValue(gen, dis) * (1 - cos(45)));

	// Compute and return random direction
	return computeDirection(phi, theta);
}

void updateSpritePositions(const std::vector<Sprite>& sprites) {
	// Preallocate memory for positions
	std::vector<float> positions;
	positions.reserve(sprites.size() * 3);

	// Populate positions vector
	for (size_t i = 0; i < sprites.size(); ++i) {
		const Sprite& s = sprites[i];
		positions.emplace_back(s.position.x);
		positions.emplace_back(s.position.y);
		positions.emplace_back(s.position.z);
	}

	// Bind VBO and upload data
	GLsizeiptr dataSize = positions.size() * sizeof(float);
	const void* bufferData = positions.data();
	GLenum usage = GL_DYNAMIC_DRAW;

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, dataSize, bufferData, usage);

	// Unbind buffer
	GLenum target = GL_ARRAY_BUFFER;
	glBindBuffer(target, 0);
}

void generateSprites(Vec3f spaceshipPosition, int spriteAmount, Vec3f direction) {
	for (int i = 0; i < spriteAmount; i++)
	{
		Sprite sprite;
		// Position
		sprite.position = spaceshipPosition;
		// Direction
		sprite.velocity = randomConicalDirection(direction);
		// How long it lasts
		sprite.lifespan = 0.5f;
		sprites.emplace_back(sprite);
	}
}

void updateSprites( float dt ) {
	// Update position and TTL
	for (auto& sprite : sprites) {
		sprite.position += sprite.velocity * dt;
		sprite.lifespan -= dt;
	}

	// Remove dead sprites
	sprites.erase(std::remove_if(sprites.begin(), sprites.end(),
		[](const Sprite& sprite) { return sprite.lifespan <= 0; }), sprites.end());
}

void renderSprites(Mat44f project2World, GLuint shader) {
	glEnable(GL_BLEND); 
	glBlendFunc(GL_SRC_ALPHA, GL_ONE); 
	glActiveTexture(GL_TEXTURE0); 
	glBindTexture(GL_TEXTURE_2D, texture);  
	glUseProgram(shader);
	glEnable(GL_PROGRAM_POINT_SIZE);
	glUniformMatrix4fv(0,1, GL_TRUE,project2World.v);
	glUniform1i(1, 0); 
	glBindVertexArray(VAO);
	glDrawArrays(GL_POINTS, 0, sprites.size()); 
	glBindVertexArray(0); 
	glDisable(GL_BLEND);
}

inline SimpleMeshData spaceship() {

	// Color definitions with obfuscated variable names
	Vec3f mainClr = { 0.35f, 0.35f, 0.3f };
	Vec3f legClr = { 1.f, 0.514f, 0.737f };
	Vec3f bodyClr = { 0.8f, 0.f, 0.404f };
	Vec3f baseClr = { 0.992f, 0.510f, 0.184f };
	Vec3f cubeClr = { 0.976f, 0.294f, 0.f };

	// Body creation
	SimpleMeshData bdyLeft = make_cone(false, size_t(64), bodyClr, make_translation({ 0.f, 2.5f, 0.f }));
	SimpleMeshData bdyRight = make_cone(false, size_t(64), bodyClr, make_translation({ 0.f, 2.5f, 0.f }) * make_rotation_z(angleToRadians(180)));
	SimpleMeshData mainBody = concatenate(bdyLeft, bdyRight);

	// Legs creation with intermediate results
	SimpleMeshData lOne = make_cylinder(true, size_t(64), legClr, make_translation({ 0.f, 2.5f, 0.f }) * make_rotation_z(angleToRadians(-45)) * make_scaling(2.f, 0.1f, 0.1f));
	SimpleMeshData tempOne = concatenate(mainBody, lOne);

	SimpleMeshData lTwo = make_cylinder(true, size_t(64), legClr, make_translation({ 0.f, 2.5f, 0.f }) * make_rotation_z(angleToRadians(-135)) * make_scaling(2.f, 0.1f, 0.1f));
	SimpleMeshData tempTwo = concatenate(tempOne, lTwo);

	SimpleMeshData lThree = make_cylinder(true, size_t(64), legClr, make_translation({ 0.f, 2.5f, 0.f }) * make_rotation_y(angleToRadians(90)) * make_rotation_z(angleToRadians(-45)) * make_scaling(2.f, 0.1f, 0.1f));
	SimpleMeshData tempThree = concatenate(tempTwo, lThree);

	SimpleMeshData lFour = make_cylinder(true, size_t(64), legClr, make_translation({ 0.f, 2.5f, 0.f }) * make_rotation_y(angleToRadians(-90)) * make_rotation_z(angleToRadians(-45)) * make_scaling(2.f, 0.1f, 0.1f));
	SimpleMeshData tempFour = concatenate(tempThree, lFour);

	// Feet creation
	SimpleMeshData fOne = make_cylinder(true, size_t(64), cubeClr, make_translation({ -1.3f, 1.f, 0.0f }) * make_scaling(0.4f, 0.4f, 0.4f));
	SimpleMeshData tempFive = concatenate(tempFour, fOne);

	SimpleMeshData fTwo = make_cylinder(true, size_t(64), cubeClr, make_translation({ 1.3f, 1.f, 0.0f }) * make_scaling(0.4f, 0.4f, 0.4f));
	SimpleMeshData tempSix = concatenate(tempFive, fTwo);

	SimpleMeshData fThree = make_cylinder(true, size_t(64), cubeClr, make_translation({ 0.f, 1.f, -1.3f }) * make_scaling(0.4f, 0.4f, 0.4f));
	SimpleMeshData tempSeven = concatenate(tempSix, fThree);

	SimpleMeshData fFour = make_cylinder(true, size_t(64), cubeClr, make_translation({ 0.0f, 1.f, 1.3f }) * make_scaling(0.4f, 0.4f, 0.4f));
	SimpleMeshData tempEight = concatenate(tempSeven, fFour);

	// Connector bars
	SimpleMeshData connOne = make_cube(baseClr, make_translation({ -0.f, 1.f, 1.2f }) * make_rotation_y(angleToRadians(90)) * make_scaling(2.4f, 0.1f, 0.1f));
	SimpleMeshData tempNine = concatenate(tempEight, connOne);

	SimpleMeshData connTwo = make_cube(baseClr, make_translation({ -1.2f, 1.f, 0.f }) * make_scaling(2.4f, 0.1f, 0.1f));
	SimpleMeshData finalSpaceship = concatenate(tempNine, connTwo);

	// Scale spaceship down
	for (int i = 0; i < finalSpaceship.positions.size(); ++i) {
		finalSpaceship.positions[i] *= 0.18;
	}

	return finalSpaceship;
}