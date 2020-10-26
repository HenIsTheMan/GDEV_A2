#include "Model.h"

Model::Model():
    storeIndices(true),
    modelPath(""),
    meshes({}),
    texTypes({}),
    allVertices({}),
    allIndices({}),
    VAO(0),
    VBO(0),
    EBO(0)
{
}

Model::Model(cstr const& fPath, const std::initializer_list<aiTextureType>& iL):
    storeIndices(true),
    modelPath(fPath),
    meshes({}),
    texTypes(iL),
    allVertices({}),
    allIndices({}),
    VAO(0),
    VBO(0),
    EBO(0)
{
}

Model::~Model(){
    if(VAO){
        glDeleteVertexArrays(1, &VAO);
    }
    if(VBO){
        glDeleteBuffers(1, &VBO);
    }
    if(EBO){
        glDeleteBuffers(1, &EBO);
    }
}

void Model::LoadModel() const{ //Load model into a DS of Assimp called a scene obj (root obj of Assimp's data interface)
    Assimp::Importer importer;
    const aiScene* const scene = importer.ReadFile(modelPath, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices);
    if(!scene || !scene->mRootNode || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE){ //If !scene || !(root node of scene) || returned data is incomplete (given by 1 of its flags)
        return (void)printf("Assimp error: %s\n", importer.GetErrorString());
    }
    ProcessNode(scene, scene->mRootNode);
}

void Model::ProcessNode(const aiScene* const& scene, const aiNode* const& node) const{ //Process all of the scene obj's nodes recursively to translate the loaded data to an arr of Mesh objs //For parent-child relation between meshes
    for(uint i = 0; i < node->mNumMeshes; ++i){ //Process all the nodes' meshes (if any)
        const aiMesh* const mesh = scene->mMeshes[node->mMeshes[i]]; //Check a node's mesh indices and retrieve the corresponding mesh by indexing the scene's mMeshes array
        meshes.emplace_back(ProcessMesh(scene, mesh)); //Store mesh obj
    }
    for(uint i = 0; i < node->mNumChildren; ++i){ //node->mNumChildren is base case/exit condition/terminating condition
        ProcessNode(scene, node->mChildren[i]); //Each node contains a set of mesh indices where each points to a specific mesh located in the scene obj
    }
}

Mesh Model::ProcessMesh(const aiScene* const& scene, const aiMesh* const& meshObj) const{
    Mesh mesh;
    mesh.vertices = new std::vector<Vertex>();
    mesh.indices = new std::vector<uint>();

    for(uint i = 0; i < meshObj->mNumVertices; ++i){ //For each vertex of the mesh...
        const aiVector3D& vertices = meshObj->mVertices[i];
        const aiColor4D* const& colours = meshObj->mColors[0];
        const aiVector3D* const& texCoords = meshObj->mTextureCoords[0];
        const aiVector3D* const& normals = meshObj->mNormals;
        const aiVector3D* const& tangents = meshObj->mTangents;
        mesh.vertices->push_back({
            glm::vec3(vertices.x, vertices.y, vertices.z),
            colours ? glm::vec4(colours[i].r, colours[i].g, colours[i].b, colours[i].a) : glm::vec4(.7f, .4f, .1f, 1.f),
            texCoords ? glm::vec2(texCoords[i].x, texCoords[i].y) : glm::vec2(0.f),
            normals ? glm::vec3(normals[i].x, normals[i].y, normals[i].z) : glm::vec3(0.f),
            tangents ? glm::vec3(tangents[i].x, tangents[i].y, tangents[i].z) : glm::vec3(0.f),
            0,
        });
    }
    for(uint i = 0; i < meshObj->mNumFaces; ++i){ //For each face of the mesh... //Each mesh has an arr of primitive faces (triangles due to the aiProcess_Triangulate post-processing option)
        for(uint j = 0; j < meshObj->mFaces[i].mNumIndices; ++j){
            mesh.indices->emplace_back(meshObj->mFaces[i].mIndices[j]);
        }
    }
    if(meshObj->mMaterialIndex >= 0){ //LoadMtlTexs //Query the mesh's mtl index to check if the mesh contains a mtl
        for(size_t i = 0; i < texTypes.size(); ++i){
            for(uint j = 0; j < scene->mMaterials[meshObj->mMaterialIndex]->GetTextureCount(texTypes[i]); ++j){
                aiString aiStr;
                scene->mMaterials[meshObj->mMaterialIndex]->GetTexture(texTypes[i], j, &aiStr);
                switch(texTypes[i]){
                    case aiTextureType_DIFFUSE:
                        mesh.AddTexMap({"Imgs/" + str{aiStr.C_Str()}, Mesh::TexType::Diffuse, 0});
                        break;
                    case aiTextureType_SPECULAR:
                        mesh.AddTexMap({"Imgs/" + str{aiStr.C_Str()}, Mesh::TexType::Spec, 0});
                        break;
                    case aiTextureType_EMISSIVE:
                        mesh.AddTexMap({"Imgs/" + str{aiStr.C_Str()}, Mesh::TexType::Emission, 0});
                        break;
                    case aiTextureType_AMBIENT:
                        mesh.AddTexMap({"Imgs/" + str{aiStr.C_Str()}, Mesh::TexType::Reflection, 0});
                        break;
                    case aiTextureType_HEIGHT:
                        mesh.AddTexMap({"Imgs/" + str{aiStr.C_Str()}, Mesh::TexType::Bump, 0});
                        break;
                }
            }
        }
    }
    return mesh;
}

void Model::AddModelMatForAll(const glm::mat4& modelMat){
    modelMatsForAll.emplace_back(modelMat);
}

//void Model::BatchRender(const int& primitive){ //Old and not working??
//    if(primitive < 0){
//        return (void)puts("Invalid primitive!\n");
//    }
//    if(!meshes.size()){
//        LoadModel();
//    }
//
//    if(!VAO){
//        glGenVertexArrays(1, &VAO);
//    }
//    glBindVertexArray(VAO);
//    if(!VBO){
//        for(const Mesh& mesh: meshes){
//            for(size_t i = 0; i < mesh.vertices->size(); ++i){
//                (*(mesh.vertices))[i].pos = glm::vec3(mesh.model * glm::vec4((*(mesh.vertices))[i].pos, 1.f));
//                allVertices.emplace_back((*(mesh.vertices))[i]);
//            }
//        }
//        glGenBuffers(1, &VBO);
//        glBindBuffer(GL_ARRAY_BUFFER, VBO);
//        glBufferData(GL_ARRAY_BUFFER, allVertices.size() * sizeof(Vertex), &allVertices[0], GL_STATIC_DRAW);
//
//        glEnableVertexAttribArray(0);
//        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, pos));
//        glEnableVertexAttribArray(1);
//        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, colour));
//        glEnableVertexAttribArray(2);
//        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, texCoords));
//        glEnableVertexAttribArray(3);
//        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, normal));
//        glEnableVertexAttribArray(4);
//        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, tangent));
//        glEnableVertexAttribArray(5);
//        glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, diffuseTexIndex));
//    } else{
//        glBindBuffer(GL_ARRAY_BUFFER, VBO);
//    }
//
//    if(storeIndices){
//        size_t verticesAmt = 0;
//        for(size_t i = 0; i < meshes.size(); ++i){
//            for(size_t j = 0; j < meshes[i].indices->size(); ++j){
//                allIndices.emplace_back((*meshes[i].indices)[j] + uint(verticesAmt));
//            }
//            verticesAmt += meshes[i].vertices->size();
//        }
//        if(allIndices.size()){
//            glGenBuffers(1, &EBO);
//        }
//        storeIndices = false;
//    }
//    if(EBO){
//        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
//        glBufferData(GL_ELEMENT_ARRAY_BUFFER, allIndices.size() * sizeof(uint), &allIndices[0], GL_STATIC_DRAW); //Alloc/Reserve a piece of GPU mem and add data into it
//        glDrawElements(primitive, (int)allIndices.size(), GL_UNSIGNED_INT, 0); //Draw/Render call/command
//    } else{
//        glDrawArrays(primitive, 0, (int)allVertices.size()); //...
//    }
//    glBindVertexArray(0);
//}

void Model::InstancedRender(ShaderProg& SP, const int& primitive){
    if(primitive < 0){
        return (void)puts("Invalid primitive!\n");
    }
    if(!meshes.size()){
        LoadModel();
    }
    const size_t& size = meshes.size();
    for(size_t i = 0; i < size; ++i){
        meshes[i].primitive = primitive;
        meshes[i].model = modelForAll;
        meshes[i].modelMats = modelMatsForAll;
        meshes[i].InstancedRender(SP);
    }
}

void Model::Render(ShaderProg& SP, const int& primitive){
    if(primitive < 0){
        return (void)puts("Invalid primitive!\n");
    }
    if(!meshes.size()){
        LoadModel();
    }
    const size_t& size = meshes.size();
    for(size_t i = 0; i < size; ++i){
        meshes[i].primitive = primitive;
        meshes[i].model = modelForAll;
        meshes[i].Render(SP);
    }
}

void Model::SetModelForAll(const glm::mat4& model){
    this->modelForAll = model;
}