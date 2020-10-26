#pragma once
#include "Mesh.h"
#include "../Global/GlobalFuncs.h"

class Model final{ //A model obj acts as a container for several mesh objs
public:
    Model();
    Model(cstr const& fPath, const std::initializer_list<aiTextureType>& iL);
    ~Model();

    void AddModelMatForAll(const glm::mat4& modelMat);
    //void BatchRender(const int& primitive);
    void InstancedRender(ShaderProg& SP, const int& primitive = GL_TRIANGLES);
    void Render(ShaderProg& SP, const int& primitive = GL_TRIANGLES);
    void SetModelForAll(const glm::mat4& model);
private:
    bool storeIndices;
    cstr modelPath;
    mutable std::vector<Mesh> meshes;
    mutable std::vector<aiTextureType> texTypes;
    std::vector<Vertex> allVertices;
    std::vector<uint> allIndices;
    glm::mat4 modelForAll;
    std::vector<glm::mat4> modelMatsForAll;
    uint VAO;
    uint VBO;
    uint EBO;

    void LoadModel() const;
    void ProcessNode(const aiScene* const& scene, const aiNode* const& node) const;
    Mesh ProcessMesh(const aiScene* const& scene, const aiMesh* const& meshObj) const;
};