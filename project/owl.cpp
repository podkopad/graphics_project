#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#define TINYGLTF_IMPLEMENTATION     // Use the implementation from stb_config.cpp
#define TINYGLTF_NO_STB_IMAGE_WRITE  // Prevents linker errors if not in config
#include <tiny_gltf.h>
#include <render/shader.h>
#include <vector>
#include <iostream>
#include <iomanip>
#define _USE_MATH_DEFINES
#include <math.h>

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

// Lighting  
static glm::vec3 lightIntensity(5e6f, 5e6f, 5e6f);
static glm::vec3 lightPosition(-275.0f, 500.0f, 800.0f);

// Animation 
static bool playAnimation = true;
static float playbackSpeed = 2.0f;

struct Owl {
	// Shader variable IDs
	GLuint mvpMatrixID;
	GLuint jointMatricesID;
	GLuint lightPositionID;
	GLuint lightIntensityID;
	GLuint programID;

	tinygltf::Model model;

	// Each VAO corresponds to each mesh primitive in the GLTF model
	struct PrimitiveObject {
		GLuint vao;
		std::map<int, GLuint> vbos;
	};
	std::vector<PrimitiveObject> primitiveObjects;

	// Skinning 
	struct SkinObject {
		// Transforms the geometry into the space of the respective joint
		std::vector<glm::mat4> inverseBindMatrices;  

		// Transforms the geometry following the movement of the joints
		std::vector<glm::mat4> globalJointTransforms;

		// Combined transforms
		std::vector<glm::mat4> jointMatrices;
	};
	std::vector<SkinObject> skinObjects;

	// Animation 
	struct SamplerObject {
		std::vector<float> input;
		std::vector<glm::vec4> output;
		int interpolation;
	};
	struct ChannelObject {
		int sampler;
		std::string targetPath;
		int targetNode;
	}; 
	struct AnimationObject {
		std::vector<SamplerObject> samplers;	// Animation data
		std::vector<ChannelObject> channels;
	};
	std::vector<AnimationObject> animationObjects;

	glm::mat4 getNodeTransform(const tinygltf::Node& node) {
		glm::mat4 transform(1.0f); 

		if (node.matrix.size() == 16) {
			transform = glm::make_mat4(node.matrix.data());
		} else {
			if (node.translation.size() == 3) {
				transform = glm::translate(transform, glm::vec3(node.translation[0], node.translation[1], node.translation[2]));
			}
			if (node.rotation.size() == 4) {
				glm::quat q(node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]);
				transform *= glm::mat4_cast(q);
			}
			if (node.scale.size() == 3) {
				transform = glm::scale(transform, glm::vec3(node.scale[0], node.scale[1], node.scale[2]));
			}
		}
		return transform;
	}

	void computeLocalNodeTransform(const tinygltf::Model& model, 
		int nodeIndex, 
		std::vector<glm::mat4> &localTransforms)
	{
		 const tinygltf::Node& node = model.nodes[nodeIndex];
		localTransforms[nodeIndex] = getNodeTransform(node);
	
		for (int childIndex : node.children) {
			computeLocalNodeTransform(model, childIndex, localTransforms);
		}
	}

	void computeGlobalNodeTransform(const tinygltf::Model& model, 
		const std::vector<glm::mat4> &localTransforms,
		int nodeIndex, const glm::mat4& parentTransform, 
		std::vector<glm::mat4> &globalTransforms)
	{
		const tinygltf::Node& node = model.nodes[nodeIndex];
		
		globalTransforms[nodeIndex] = parentTransform * localTransforms[nodeIndex];
		
		for (int childIndex : node.children) {
			computeGlobalNodeTransform(model, localTransforms, childIndex, 
									globalTransforms[nodeIndex], globalTransforms);
		}
	}

	std::vector<SkinObject> prepareSkinning(const tinygltf::Model &model) {
		std::vector<SkinObject> skinObjects;

		// In our Blender exporter, the default number of joints that may influence a vertex is set to 4, just for convenient implementation in shaders.

		for (size_t i = 0; i < model.skins.size(); i++) {
			SkinObject skinObject;

			const tinygltf::Skin &skin = model.skins[i];
			const tinygltf::Accessor &accessor = model.accessors[skin.inverseBindMatrices];
			assert(accessor.type == TINYGLTF_TYPE_MAT4);
			const tinygltf::BufferView &bufferView = model.bufferViews[accessor.bufferView];
			const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];
			const float *ptr = reinterpret_cast<const float *>(
				buffer.data.data() + accessor.byteOffset + bufferView.byteOffset);
			
			skinObject.inverseBindMatrices.resize(accessor.count);
			for (size_t j = 0; j < accessor.count; j++) {
				float m[16];
				memcpy(m, ptr + j * 16, 16 * sizeof(float));
				skinObject.inverseBindMatrices[j] = glm::make_mat4(m);
			}

			assert(skin.joints.size() == accessor.count);

			skinObject.globalJointTransforms.resize(skin.joints.size());
			skinObject.jointMatrices.resize(skin.joints.size());

			// ----------------------------------------------
			// TODO: your code here to compute joint matrices
			// ----------------------------------------------

			// Compute local transforms for ALL nodes
			std::vector<glm::mat4> localTransforms(model.nodes.size());
			for (size_t j = 0; j < model.nodes.size(); ++j) {
				localTransforms[j] = getNodeTransform(model.nodes[j]);
			}

			// Compute global transforms starting from root
			std::vector<glm::mat4> globalTransforms(model.nodes.size());
			int skeletonRoot = (skin.skeleton >= 0) ? skin.skeleton : skin.joints[0];
			glm::mat4 parentTransform(1.0f);
			computeGlobalNodeTransform(model, localTransforms, skeletonRoot, parentTransform, globalTransforms);

			// Extract joint transforms and compute joint matrices
			for (size_t j = 0; j < skin.joints.size(); ++j) {
				int nodeIndex = skin.joints[j];
				skinObject.globalJointTransforms[j] = globalTransforms[nodeIndex];
				skinObject.jointMatrices[j] = skinObject.globalJointTransforms[j] * skinObject.inverseBindMatrices[j];
			}

			// ----------------------------------------------

			skinObjects.push_back(skinObject);
		}
		return skinObjects;
	}

	int findKeyframeIndex(const std::vector<float>& times, float animationTime) 
	{
		int left = 0;
		int right = times.size() - 1;

		while (left <= right) {
			int mid = (left + right) / 2;

			if (mid + 1 < times.size() && times[mid] <= animationTime && animationTime < times[mid + 1]) {
				return mid;
			}
			else if (times[mid] > animationTime) {
				right = mid - 1;
			}
			else { // animationTime >= times[mid + 1]
				left = mid + 1;
			}
		}

		// Target not found
		return times.size() - 2;
	}

	std::vector<AnimationObject> prepareAnimation(const tinygltf::Model &model) 
	{
		std::vector<AnimationObject> animationObjects;
		for (const auto &anim : model.animations) {
			AnimationObject animationObject;
			
			for (const auto &sampler : anim.samplers) {
				SamplerObject samplerObject;

				const tinygltf::Accessor &inputAccessor = model.accessors[sampler.input];
				const tinygltf::BufferView &inputBufferView = model.bufferViews[inputAccessor.bufferView];
				const tinygltf::Buffer &inputBuffer = model.buffers[inputBufferView.buffer];

				assert(inputAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
				assert(inputAccessor.type == TINYGLTF_TYPE_SCALAR);

				// Input (time) values
				samplerObject.input.resize(inputAccessor.count);

				const unsigned char *inputPtr = &inputBuffer.data[inputBufferView.byteOffset + inputAccessor.byteOffset];
				const float *inputBuf = reinterpret_cast<const float*>(inputPtr);

				// Read input (time) values
				int stride = inputAccessor.ByteStride(inputBufferView);
				for (size_t i = 0; i < inputAccessor.count; ++i) {
					samplerObject.input[i] = *reinterpret_cast<const float*>(inputPtr + i * stride);
				}
				
				const tinygltf::Accessor &outputAccessor = model.accessors[sampler.output];
				const tinygltf::BufferView &outputBufferView = model.bufferViews[outputAccessor.bufferView];
				const tinygltf::Buffer &outputBuffer = model.buffers[outputBufferView.buffer];

				assert(outputAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

				const unsigned char *outputPtr = &outputBuffer.data[outputBufferView.byteOffset + outputAccessor.byteOffset];
				const float *outputBuf = reinterpret_cast<const float*>(outputPtr);

				int outputStride = outputAccessor.ByteStride(outputBufferView);
				
				// Output values
				samplerObject.output.resize(outputAccessor.count);
				
				for (size_t i = 0; i < outputAccessor.count; ++i) {

					if (outputAccessor.type == TINYGLTF_TYPE_VEC3) {
						memcpy(&samplerObject.output[i], outputPtr + i * 3 * sizeof(float), 3 * sizeof(float));
					} else if (outputAccessor.type == TINYGLTF_TYPE_VEC4) {
						memcpy(&samplerObject.output[i], outputPtr + i * 4 * sizeof(float), 4 * sizeof(float));
					} 

				}

				animationObject.samplers.push_back(samplerObject);			
			}

			animationObjects.push_back(animationObject);
		}
		return animationObjects;
	}

	void updateAnimation(
    const tinygltf::Model &model, 
    const tinygltf::Animation &anim, 
    const AnimationObject &animationObject, 
    float time,
    std::vector<glm::mat4> &nodeTransforms) 
{
    for (const auto &channel : anim.channels) {
        int targetNodeIndex = channel.target_node;
        
        // SAFETY: Prevents the "vector issue" crash
        if (targetNodeIndex < 0 || targetNodeIndex >= (int)nodeTransforms.size()) continue;

        const auto &sampler = anim.samplers[channel.sampler];
        const std::vector<float> &times = animationObject.samplers[channel.sampler].input;
        if (times.empty()) continue;

        float animationTime = fmod(time, times.back());
        int keyframeIndex = findKeyframeIndex(times, animationTime); 

        // Ensure we don't look past the end of the array
        if (keyframeIndex >= (int)times.size() - 1) keyframeIndex = (int)times.size() - 2;

        float t = (animationTime - times[keyframeIndex]) / (times[keyframeIndex + 1] - times[keyframeIndex]);

        const tinygltf::Accessor &outputAccessor = model.accessors[sampler.output];
        const tinygltf::BufferView &outputBufferView = model.bufferViews[outputAccessor.bufferView];
        const tinygltf::Buffer &outputBuffer = model.buffers[outputBufferView.buffer];
        const unsigned char *outputPtr = &outputBuffer.data[outputBufferView.byteOffset + outputAccessor.byteOffset];

        // START with current matrix data so T, R, and S channels don't fight
        glm::vec3 translation;
        glm::quat rotation;
        glm::vec3 scale;

        glm::mat4 currentMat = nodeTransforms[targetNodeIndex];
        translation = glm::vec3(currentMat[3]);
        scale = glm::vec3(
            glm::length(glm::vec3(currentMat[0])),
            glm::length(glm::vec3(currentMat[1])),
            glm::length(glm::vec3(currentMat[2]))
        );
        
        // Safety: Avoid division by zero which causes "not rendering" (NaN)
        glm::mat3 rotMat = glm::mat3(
            glm::vec3(currentMat[0]) / (scale.x > 0.001f ? scale.x : 1.0f),
            glm::vec3(currentMat[1]) / (scale.y > 0.001f ? scale.y : 1.0f),
            glm::vec3(currentMat[2]) / (scale.z > 0.001f ? scale.z : 1.0f)
        );
        rotation = glm::quat_cast(rotMat);

        if (channel.target_path == "translation") {
            glm::vec3 t0, t1;
            memcpy(&t0, outputPtr + keyframeIndex * 3 * sizeof(float), 3 * sizeof(float));
            memcpy(&t1, outputPtr + (keyframeIndex + 1) * 3 * sizeof(float), 3 * sizeof(float));
            translation = glm::mix(t0, t1, t);
        } 
        else if (channel.target_path == "rotation") {
            glm::quat r0, r1;
            memcpy(&r0, outputPtr + keyframeIndex * 4 * sizeof(float), 4 * sizeof(float));
            memcpy(&r1, outputPtr + (keyframeIndex + 1) * 4 * sizeof(float), 4 * sizeof(float));
            rotation = glm::slerp(r0, r1, t);
        } 
        else if (channel.target_path == "scale") {
            glm::vec3 s0, s1;
            memcpy(&s0, outputPtr + keyframeIndex * 3 * sizeof(float), 3 * sizeof(float));
            memcpy(&s1, outputPtr + (keyframeIndex + 1) * 3 * sizeof(float), 3 * sizeof(float));
            scale = glm::mix(s0, s1, t);
        }

        nodeTransforms[targetNodeIndex] = glm::translate(glm::mat4(1.0f), translation) * glm::mat4_cast(rotation) * glm::scale(glm::mat4(1.0f), scale);
    }
}

void updateSkinning(const std::vector<glm::mat4> &nodeTransforms) {
    if (skinObjects.empty() || model.skins.empty()) return;

    const tinygltf::Skin &skin = model.skins[0];
    std::vector<glm::mat4> globalTransforms(model.nodes.size(), glm::mat4(1.0f));
    
    // Process every root node in the scene to ensure nothing is missed
    const tinygltf::Scene &scene = model.scenes[model.defaultScene];
    for (int rootIndex : scene.nodes) {
        if (rootIndex < 0 || rootIndex >= model.nodes.size()) continue;
        computeGlobalNodeTransform(model, nodeTransforms, rootIndex, glm::mat4(1.0f), globalTransforms);
    }

    // Update the matrices actually used by the shader
    for (size_t j = 0; j < skin.joints.size(); j++) {
        int nodeIndex = skin.joints[j];
        if (nodeIndex >= 0 && nodeIndex < globalTransforms.size()) {
            skinObjects[0].jointMatrices[j] = globalTransforms[nodeIndex] * skinObjects[0].inverseBindMatrices[j];
        }
    }
}

void update(float time) {
    // 1. Safety Checks: If the model is empty or not loaded, stop.
    if (model.nodes.empty()) return;
    if (model.animations.empty() || animationObjects.empty()) return;
    if (skinObjects.empty()) return;

    // 2. Setup local transforms
    std::vector<glm::mat4> nodeTransforms(model.nodes.size());
    for (size_t i = 0; i < model.nodes.size(); ++i) {
        nodeTransforms[i] = getNodeTransform(model.nodes[i]);
    }

    // 3. Apply animation to the local transforms
    // Note: We use model.animations[0] and animationObjects[0]
    updateAnimation(model, model.animations[0], animationObjects[0], time, nodeTransforms);
    
    // 4. Update the skeleton based on those new transforms
    updateSkinning(nodeTransforms);
}
	bool loadModel(tinygltf::Model &model, const char *filename) {
		tinygltf::TinyGLTF loader;
		std::string err;
		std::string warn;
		

		bool res = loader.LoadASCIIFromFile(&model, &err, &warn, filename);
		if (!warn.empty()) {
			std::cout << "WARN: " << warn << std::endl;
		}

		if (!err.empty()) {
			std::cout << "ERR: " << err << std::endl;
		}

		if (!res)
			std::cout << "Failed to load glTF: " << filename << std::endl;
		else
			std::cout << "Loaded glTF: " << filename << std::endl;

		return res;
	}

	void initialize() {
		// Modify your path if needed
		if (!loadModel(model, "C:/Users/User/Desktop/graphics/final proj/project/owl/owl.gltf")) {
			return;
		}

		// Prepare buffers for rendering 
		primitiveObjects = bindModel(model);

		// Prepare joint matrices
		skinObjects = prepareSkinning(model);

		// Prepare animation data 
		animationObjects = prepareAnimation(model);

		// Create and compile our GLSL program from the shaders
		programID = LoadShadersFromFile(
    "C:/Users/User/Desktop/graphics/final proj/project/owl.vert",
    "C:/Users/User/Desktop/graphics/final proj/project/owl.frag"
);
		if (programID == 0)
		{
			std::cerr << "Failed to load shaders." << std::endl;
		}

		// Get a handle for GLSL variables
		mvpMatrixID = glGetUniformLocation(programID, "MVP");
		jointMatricesID = glGetUniformLocation(programID, "jointMatrices"); 
		lightPositionID = glGetUniformLocation(programID, "lightPosition");
		lightIntensityID = glGetUniformLocation(programID, "lightIntensity");
	}

	void bindMesh(std::vector<PrimitiveObject> &primitiveObjects,
				tinygltf::Model &model, tinygltf::Mesh &mesh) {

		std::map<int, GLuint> vbos;
		for (size_t i = 0; i < model.bufferViews.size(); ++i) {
			const tinygltf::BufferView &bufferView = model.bufferViews[i];

			int target = bufferView.target;
			
			if (bufferView.target == 0) { 
				// The bufferView with target == 0 in our model refers to 
				// the skinning weights, for 25 joints, each 4x4 matrix (16 floats), totaling to 400 floats or 1600 bytes. 
				// So it is considered safe to skip the warning.
				//std::cout << "WARN: bufferView.target is zero" << std::endl;
				continue;
			}

			const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];
			GLuint vbo;
			glGenBuffers(1, &vbo);
			glBindBuffer(target, vbo);
			glBufferData(target, bufferView.byteLength,
						&buffer.data.at(0) + bufferView.byteOffset, GL_STATIC_DRAW);
			
			vbos[i] = vbo;
		}

		// Each mesh can contain several primitives (or parts), each we need to 
		// bind to an OpenGL vertex array object
		for (size_t i = 0; i < mesh.primitives.size(); ++i) {

			tinygltf::Primitive primitive = mesh.primitives[i];
			tinygltf::Accessor indexAccessor = model.accessors[primitive.indices];

			GLuint vao;
			glGenVertexArrays(1, &vao);
			glBindVertexArray(vao);

			for (auto &attrib : primitive.attributes) {
				tinygltf::Accessor accessor = model.accessors[attrib.second];
				int byteStride =
					accessor.ByteStride(model.bufferViews[accessor.bufferView]);
				glBindBuffer(GL_ARRAY_BUFFER, vbos[accessor.bufferView]);

				int size = 1;
				if (accessor.type != TINYGLTF_TYPE_SCALAR) {
					size = accessor.type;
				}

				int vaa = -1;
				if (attrib.first.compare("POSITION") == 0) vaa = 0;
				if (attrib.first.compare("NORMAL") == 0) vaa = 1;
				if (attrib.first.compare("TEXCOORD_0") == 0) vaa = 2;
				if (attrib.first.compare("JOINTS_0") == 0) vaa = 3;
				if (attrib.first.compare("WEIGHTS_0") == 0) vaa = 4;
				if (vaa > -1) {
					glEnableVertexAttribArray(vaa);
					glVertexAttribPointer(vaa, size, accessor.componentType,
										accessor.normalized ? GL_TRUE : GL_FALSE,
										byteStride, BUFFER_OFFSET(accessor.byteOffset));
				}
			}

			// Record VAO for later use
			PrimitiveObject primitiveObject;
			primitiveObject.vao = vao;
			primitiveObject.vbos = vbos;
			primitiveObjects.push_back(primitiveObject);

			glBindVertexArray(0);
		}
	}

	void bindModelNodes(std::vector<PrimitiveObject> &primitiveObjects, 
						tinygltf::Model &model,
						tinygltf::Node &node) {
		// Bind buffers for the current mesh at the node
		if ((node.mesh >= 0) && (node.mesh < model.meshes.size())) {
			bindMesh(primitiveObjects, model, model.meshes[node.mesh]);
		}

		// Recursive into children nodes
		for (size_t i = 0; i < node.children.size(); i++) {
			assert((node.children[i] >= 0) && (node.children[i] < model.nodes.size()));
			bindModelNodes(primitiveObjects, model, model.nodes[node.children[i]]);
		}
	}

	std::vector<PrimitiveObject> bindModel(tinygltf::Model &model) {
		std::vector<PrimitiveObject> primitiveObjects;

		const tinygltf::Scene &scene = model.scenes[model.defaultScene];
		for (size_t i = 0; i < scene.nodes.size(); ++i) {
			assert((scene.nodes[i] >= 0) && (scene.nodes[i] < model.nodes.size()));
			bindModelNodes(primitiveObjects, model, model.nodes[scene.nodes[i]]);
		}

		return primitiveObjects;
	}

	void drawMesh(const std::vector<PrimitiveObject> &primitiveObjects,
				tinygltf::Model &model, tinygltf::Mesh &mesh) {
		
		for (size_t i = 0; i < mesh.primitives.size(); ++i) 
		{
			GLuint vao = primitiveObjects[i].vao;
			std::map<int, GLuint> vbos = primitiveObjects[i].vbos;

			glBindVertexArray(vao);

			tinygltf::Primitive primitive = mesh.primitives[i];
			tinygltf::Accessor indexAccessor = model.accessors[primitive.indices];

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos.at(indexAccessor.bufferView));

			glDrawElements(primitive.mode, indexAccessor.count,
						indexAccessor.componentType,
						BUFFER_OFFSET(indexAccessor.byteOffset));

			glBindVertexArray(0);
		}
	}

	void drawModelNodes(const std::vector<PrimitiveObject>& primitiveObjects,
						tinygltf::Model &model, tinygltf::Node &node) {
		// Draw the mesh at the node, and recursively do so for children nodes
		if ((node.mesh >= 0) && (node.mesh < model.meshes.size())) {
			drawMesh(primitiveObjects, model, model.meshes[node.mesh]);
		}
		for (size_t i = 0; i < node.children.size(); i++) {
			drawModelNodes(primitiveObjects, model, model.nodes[node.children[i]]);
		}
	}
	void drawModel(const std::vector<PrimitiveObject>& primitiveObjects,
				tinygltf::Model &model) {
		// Draw all nodes
		const tinygltf::Scene &scene = model.scenes[model.defaultScene];
		for (size_t i = 0; i < scene.nodes.size(); ++i) {
			drawModelNodes(primitiveObjects, model, model.nodes[scene.nodes[i]]);
		}
	}

	void render(glm::mat4 cameraMatrix) {
    // If the model didn't load, don't try to render
    if (primitiveObjects.empty()) return;

    glUseProgram(programID);
    
    glm::mat4 mvp = cameraMatrix;
    glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &mvp[0][0]);

    // SAFETY CHECK: Only set uniforms if skinObjects actually has data
    if (!skinObjects.empty() && !skinObjects[0].jointMatrices.empty()) {
        glUniformMatrix4fv(jointMatricesID, (GLsizei)skinObjects[0].jointMatrices.size(), 
                   GL_FALSE, glm::value_ptr(skinObjects[0].jointMatrices[0]));
    }

    glUniform3fv(lightPositionID, 1, &lightPosition[0]);
    glUniform3fv(lightIntensityID, 1, &lightIntensity[0]);

    drawModel(primitiveObjects, model);
}
}; 
