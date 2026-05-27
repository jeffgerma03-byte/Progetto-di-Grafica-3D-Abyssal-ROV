#ifndef MODELLOADER_H
#define MODELLOADER_H

#include <vector>
#include <string>
#include <fstream>
#include <iostream>

/*
 * Classe ModelLoader
 * Un parser custom minimale per file Wavefront (.obj).
 * Decodifica la geometria 3D, le coordinate UV e i vettori Normali
 * estraendoli da testo puro e impacchettandoli in un singolo array lineare 
 * compatibile con la memoria GPU (Vertex Buffer Objects).
 */
class ModelLoader {
public:
    static bool loadOBJ(const std::string& path, std::vector<float>& out_data) {
        
        std::vector<float> temp_vertices;
        std::vector<float> temp_uvs;
        std::vector<float> temp_normals;
        
        std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;

        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "Errore: Impossibile aprire il file " << path << std::endl;
            return false;
        }

        std::string type;
        while (file >> type) {
            if (type == "v") {
                float x, y, z;
                file >> x >> y >> z;
                temp_vertices.push_back(x);
                temp_vertices.push_back(y);
                temp_vertices.push_back(z);
            } 
            else if (type == "vt") {
                float u, v;
                file >> u >> v;
                temp_uvs.push_back(u);
                temp_uvs.push_back(v);
            } 
            else if (type == "vn") {
                float nx, ny, nz;
                file >> nx >> ny >> nz;
                temp_normals.push_back(nx);
                temp_normals.push_back(ny);
                temp_normals.push_back(nz);
            } 
            else if (type == "f") {
                unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
                char slash; 
                
                for (int i = 0; i < 3; i++) {
                    file >> vertexIndex[i] >> slash >> uvIndex[i] >> slash >> normalIndex[i];
                    
                    vertexIndices.push_back(vertexIndex[i]);
                    uvIndices.push_back(uvIndex[i]);
                    normalIndices.push_back(normalIndex[i]);
                }
            } 
            else {
                std::string dummy;
                std::getline(file, dummy);
            }
        }
        file.close();

        for (size_t i = 0; i < vertexIndices.size(); i++) {
            
            unsigned int vIndex = vertexIndices[i] - 1;
            unsigned int uvIndex = uvIndices[i] - 1;
            unsigned int nIndex = normalIndices[i] - 1;

            out_data.push_back(temp_vertices[vIndex * 3 + 0]);
            out_data.push_back(temp_vertices[vIndex * 3 + 1]);
            out_data.push_back(temp_vertices[vIndex * 3 + 2]);

            out_data.push_back(temp_uvs[uvIndex * 2 + 0]);
            out_data.push_back(temp_uvs[uvIndex * 2 + 1]);

            out_data.push_back(temp_normals[nIndex * 3 + 0]);
            out_data.push_back(temp_normals[nIndex * 3 + 1]);
            out_data.push_back(temp_normals[nIndex * 3 + 2]);
        }

        return true;
    }
};

#endif