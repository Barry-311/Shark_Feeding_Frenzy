#ifndef FBXMODEL_HPP
#define FBXMODEL_HPP

#include <vector>
#include <string>
#include <iostream>

#include <glm.hpp>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glad/glad.h>

class FBXModel {
public:
    struct ModelData {
        size_t pointCount = 0;
        std::vector<glm::vec3> vertices;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec2> textureCoords;
        std::vector<int> meshIDs; // 为每个顶点记录其所属的 mesh ID

        ModelData() = default;
    };

    struct MeshInfo {
        size_t vertexCount = 0;
        bool hasTexture = false;
    };

    struct MeshData {
        GLuint vao = 0;
        GLuint vertexBuffer = 0;
        GLuint normalBuffer = 0;
        GLuint textureBuffer = 0;
        GLuint meshIDBuffer = 0; // 记录 meshID 的 buffer
        size_t vertexCount = 0;
    };

private:
    ModelData meshData;                // 模型的顶点、法线、纹理坐标等数据
    std::vector<MeshInfo> meshMeshes; // 存储每个 mesh 的信息

public:
    FBXModel() = default;

    bool loadFromFile(const std::string& fileName) {
        const aiScene* scene = aiImportFile(
            fileName.c_str(),
            aiProcess_Triangulate | aiProcess_PreTransformVertices
        );

        if (!scene) {
            std::cerr << "Error loading model: " << fileName << "\n";
            return false;
        }

        for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
            const aiMesh* mesh = scene->mMeshes[i];
            MeshInfo meshInfo;
            meshInfo.vertexCount = mesh->mNumVertices;
            meshInfo.hasTexture = mesh->HasTextureCoords(0);
            meshMeshes.push_back(meshInfo);

            meshData.pointCount += mesh->mNumVertices;

            for (unsigned int v = 0; v < mesh->mNumVertices; ++v) {
                if (mesh->HasPositions()) {
                    const aiVector3D* vp = &(mesh->mVertices[v]);
                    meshData.vertices.push_back(glm::vec3(vp->x, vp->y, vp->z));
                }
                if (mesh->HasNormals()) {
                    const aiVector3D* vn = &(mesh->mNormals[v]);
                    meshData.normals.push_back(glm::vec3(vn->x, vn->y, vn->z));
                }
                if (mesh->HasTextureCoords(0)) {
                    const aiVector3D* vt = &(mesh->mTextureCoords[0][v]);
                    meshData.textureCoords.push_back(glm::vec2(vt->x, vt->y));
                }
                meshData.meshIDs.push_back(i); // 记录当前顶点所属的 mesh ID
            }
        }

        aiReleaseImport(scene);
        return true;
    }

    const ModelData& getModelData() const {
        return meshData;
    }

    void generateObjectBufferMesh(const ModelData& modelData, MeshData& meshData) {
        meshData.vertexCount = modelData.pointCount;

        GLuint buffers[4];
        glGenVertexArrays(1, &meshData.vao);
        glGenBuffers(4, buffers);

        meshData.vertexBuffer = buffers[0];
        meshData.normalBuffer = buffers[1];
        meshData.textureBuffer = buffers[2];
        meshData.meshIDBuffer = buffers[3];

        glBindVertexArray(meshData.vao);

        // 顶点位置
        glBindBuffer(GL_ARRAY_BUFFER, meshData.vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, modelData.vertices.size() * sizeof(glm::vec3), modelData.vertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(0);

        // 法线
        glBindBuffer(GL_ARRAY_BUFFER, meshData.normalBuffer);
        glBufferData(GL_ARRAY_BUFFER, modelData.normals.size() * sizeof(glm::vec3), modelData.normals.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(1);

        // 纹理坐标（如果可用）
        if (!modelData.textureCoords.empty()) {
            glBindBuffer(GL_ARRAY_BUFFER, meshData.textureBuffer);
            glBufferData(GL_ARRAY_BUFFER, modelData.textureCoords.size() * sizeof(glm::vec2), modelData.textureCoords.data(), GL_STATIC_DRAW);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
            glEnableVertexAttribArray(2);
        }

        // Mesh ID
        glBindBuffer(GL_ARRAY_BUFFER, meshData.meshIDBuffer);
        glBufferData(GL_ARRAY_BUFFER, modelData.meshIDs.size() * sizeof(int), modelData.meshIDs.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(3, 1, GL_INT, 0, 0, nullptr);
        glEnableVertexAttribArray(3);

        glBindVertexArray(0);
    }

    void printMeshInfo() const {
        std::cout << "Model contains " << meshMeshes.size() << " mesh(es):" << std::endl;

        for (size_t i = 0; i < meshMeshes.size(); ++i) {
            const auto& mesh = meshMeshes[i];
            std::cout << "  Mesh " << i + 1 << ":" << std::endl;
            std::cout << "    Vertex Count: " << mesh.vertexCount << std::endl;
            std::cout << "    Texture Coordinates: " << (mesh.hasTexture ? "Yes" : "No") << std::endl;
        }

        std::cout << "Total vertices across all meshes: " << meshData.pointCount << std::endl;
    }
};

#endif // FBXMODEL_HPP
