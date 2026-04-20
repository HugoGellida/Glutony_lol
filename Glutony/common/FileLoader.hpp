#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include "common/gameobject/component/Mesh.hpp"
#include <glm/glm.hpp>

namespace fileLoader
{
    using namespace std;

    enum EXTENSION
    {
        INVALID,
        OBJ,
        OFF
    };

    inline EXTENSION getExtension(std::string const & filename)
    {
        if (filename[filename.size() - 4] == '.' && filename[filename.size() - 3] == 'o' && filename[filename.size() - 2] == 'b' && filename[filename.size() - 1] == 'j')
            return OBJ;
        else if (filename[filename.size() - 4] == '.' && filename[filename.size() - 3] == 'o' && filename[filename.size() - 2] == 'f' && filename[filename.size() - 1] == 'f')
            return OFF;
        else
            return INVALID;
    }




    /*
    static component::Mesh * loadOFFN (
        std::string const & filename
    ) {
        
        std::ifstream myfile;
        myfile.open(filename.c_str());
        if (!myfile.is_open())
        {
            throw "mesh cannot be opened";
        }

        std::string magic_s;

        myfile >> magic_s;

        if( magic_s != "OFF" )
        {
            std::cout << magic_s << " != OFF :   We handle ONLY *.off files." << std::endl;
            myfile.close();
            exit(1);
        }

        int n_vertices , n_faces , dummy_int;
        myfile >> n_vertices >> n_faces >> dummy_int;

        vector<Vec3f> verts = vector<Vec3f>(n_vertices);

        vector<Vec3f> norms = vector<Vec3f>(n_vertices);
        vector<uint> tri;
        tri.reserve(n_faces * 6);

        for( int v = 0 ; v < n_vertices ; ++v )
        {
            float x , y , z , xn, yn, zn;

            myfile >> x >> y >> z >> xn >> yn >> zn;
            verts.push_back(Vec3f(x, y, z));   
            norms.push_back(Vec3f(x, y, z));   
        }
        for( int f = 0 ; f < n_faces ; ++f )
        {
            int n_vertices_on_face;
            myfile >> n_vertices_on_face;

            if( n_vertices_on_face == 3 )
            {
                unsigned int _v1 , _v2 , _v3;
                myfile >> _v1 >> _v2 >> _v3;

                tri.push_back(_v1);
                tri.push_back(_v2);
                tri.push_back(_v3);
            }
            else if( n_vertices_on_face == 4 )
            {
                unsigned int _v1 , _v2 , _v3 , _v4;
                myfile >> _v1 >> _v2 >> _v3 >> _v4;

                tri.push_back(_v1);
                tri.push_back(_v2);
                tri.push_back(_v3);
                tri.push_back(_v1);
                tri.push_back(_v3);
                tri.push_back(_v4);
            }
            else
            {
                std::cout << "We handle ONLY *.off files with 3 or 4 vertices per face" << std::endl;
                myfile.close();
                exit(1);
            }
            
        }
        component::Mesh * mesh = new component::Mesh(verts.size(), tri.size() / 3);
        for (uint i = 0; i < verts.size(); i++)
            mesh -> setVertice(i, verts[i], norms[i]);
        for (uint i = 0; i < tri.size() / 3; i++)
            mesh -> setTriangle(i, i*3, i*3+1, i*3+2);

        std::cout << "model loaded." << std::endl
                << " Info :" << std::endl
                << "  - name : " << filename << std::endl
                << "  - v: " << verts.size() << std::endl
                << "  - t: " << tri.size() / 3 << std::endl;
        return mesh;
    }*/

    static component::Mesh * loadOFF (
        std::string const & filename
    ) {
        
        std::ifstream myfile;
        myfile.open(filename.c_str());
        if (!myfile.is_open())
        {
            std::cerr << "mesh cannot be opened" << std::endl;
            exit(1);
        }

        std::string magic_s;

        myfile >> magic_s;

        if( magic_s != "OFF" )
        {
            std::cout << magic_s << " != OFF :   We handle ONLY *.off files." << std::endl;
            myfile.close();
            exit(1);
        }

        int n_vertices , n_faces , dummy_int;
        myfile >> n_vertices >> n_faces >> dummy_int;

        vector<float> verts = vector<float>(n_vertices * 3);

        vector<uint> tri;
        tri.reserve(n_faces * 6);

        for( int v = 0 ; v < n_vertices ; v++ )
        {
            float x , y , z, _x, _y , _z;

            myfile >> x >> y >> z >> _x >> _y >> _z;
            verts[v*3] = x;
            verts[v*3+1] = y;
            verts[v*3+2] = z;
        }
        for( int f = 0 ; f < n_faces ; f++ )
        {
            int n_vertices_on_face;
            myfile >> n_vertices_on_face;

            if( n_vertices_on_face == 3 )
            {
                unsigned int _v1 , _v2 , _v3;
                myfile >> _v1 >> _v2 >> _v3;

                tri.push_back(_v1);
                tri.push_back(_v2);
                tri.push_back(_v3);
            }
            else if( n_vertices_on_face == 4 )
            {
                unsigned int _v1 , _v2 , _v3 , _v4;
                myfile >> _v1 >> _v2 >> _v3 >> _v4;

                tri.push_back(_v1);
                tri.push_back(_v2);
                tri.push_back(_v3);
                tri.push_back(_v1);
                tri.push_back(_v3);
                tri.push_back(_v4);
            }
            else
            {
                std::cout << "We handle ONLY *.off files with 3 or 4 vertices per face" << std::endl;
                myfile.close();
                exit(1);
            }
            
        }
        component::Mesh * mesh = new component::Mesh(verts.size() / 3, tri.size() / 3);
        for (uint i = 0; i < verts.size() / 3; i++)
            mesh -> setVertice(i, glm::vec3(verts[i*3], verts[i*3+1], verts[i*3+2]));
        for (uint i = 0; i < tri.size() / 3; i++)
            mesh -> setTriangle(i, tri[i*3], tri[i*3+1], tri[i*3+2]);

        mesh -> computeNormals();

        std::cout << "model loaded." << std::endl
                << " Info :" << std::endl
                << "  - name : " << filename << std::endl
                << "  - v: " << verts.size() / 3 << std::endl
                << "  - t: " << tri.size() / 3 << std::endl;
        return mesh;
    }





    inline float readFloat(uint & start, string str)
    {
        string buf = "";
        while (start < str.size())
            if ((str[start] == ' ' ||str[start] == '\r') && !buf.size() == 0)
                return atof(buf.c_str());
            else
                buf.push_back(str[start++]);
        return atof(buf.c_str());
    }
    inline uint readFUInt(uint & start, string & str)
    {
        string buf = "";
        while(start < str.size())
            if (str[start] == ' ' || str[start] == '/')
                return atoi(buf.c_str());
            else
                buf.push_back(str[start++]);
        return atoi(buf.c_str());
    }

    inline void readFace(uint & start, string str, uint & vert)
    {
        vert = readFUInt(start, str);
        if (start < str.size() && str[start] != ' ') // offset to next space, ignore potential tex + norm values
        {
            while (start < str.size() && str[start] != ' ')
                start++;
        }
    }

    inline bool __objLineQuad(uint start, string str)
    {
        uint spaceCount = 0;
        for (uint i = start; i < str.size() - 1; i++)
            if (str[i] == ' ' && str[i+1] != ' ')
                spaceCount++;
        return spaceCount == 4;
    }
    
    inline void readFace(uint & start, string & str, uint & vert, uint &tex, uint & norm)
    {
        bool containTexNorm = false;
        for (uint i = start; i < str.size(); i++)
            if (str[i] == '/')
                containTexNorm = true;
        if (containTexNorm)
        {
            vert = readFUInt(start, str);
            tex = readFUInt(++start, str);
            norm = readFUInt(++start, str);
        }
        else
        {
            vert = readFUInt(start, str);
        }
    }
    
    static component::Mesh * ObjLoadFast(string path)
    {
        // type + " " -> is "v "? -> beginFloat => [- + [0-9]] + " " -> endFloat // repeat * 2 more => to vec3
        std::ifstream fileStream(path, std::ios::in);

        if (!fileStream.is_open()) {
            throw "File could not be opened";
        }

        uint vStride = 0;
        uint tStride = 0;
        // v vn vt f , g
        std::string line = "";
        while (!fileStream.eof()) { 
            getline(fileStream, line);
            if (line.size() > 2)
            {
                if (line[0] == 'v' && line[1] == ' ')
                    vStride++;
                if (line[0] == 'f' && line[1] == ' ')
                {
                    if (__objLineQuad(2, line))
                        tStride+=2;
                    else
                        tStride++;
                }
            }
        }
        fileStream.close();
        std::ifstream fileStream2(path, std::ios::in);
        // now reread to actually collect and parse.
        if (!fileStream2.is_open()) {
            throw "File could not be opened";
        }
        component::Mesh * mesh = new component::Mesh(vStride, tStride);
        line = "";
        uint v = 0;
        uint t = 0;
        while (!fileStream2.eof())
        {
            getline(fileStream2, line);
            if (line.size() > 2)
            {
                if (line[0] == 'v' && line[1] == ' ')
                {
                    uint i = 2;
                    float x = readFloat(i, line);
                    float y = readFloat(++i, line);
                    float z = readFloat(++i, line);
                    mesh -> setVertice(v++, glm::vec3(x, y, z));
                }
                if (line[0] == 'f' && line[1] == ' ')
                {
                    uint i = 2;
                    uint t0, t1, t2;
                    bool quad = __objLineQuad(i, line);
                    readFace(i, line, t0);
                    readFace(++i, line, t1);
                    readFace(++i, line, t2);
                    mesh -> setTriangle(t++, t0-1, t1-1, t2-1);
                    if (quad)
                    {
                        uint t3;
                        readFace(++i, line, t3);
                        mesh -> setTriangle(t++, t0-1, t2-1, t3-1);
                    }
                }
            }
        }
        fileStream2.close();
        return mesh;
    }

    struct ObjVertexKey
    {
        int positionIndex = -1;
        int texcoordIndex = -1;
        int normalIndex = -1;
    };

    inline int resolveObjIndex(const int rawIndex, const size_t count)
    {
        if (rawIndex > 0)
            return rawIndex - 1;
        if (rawIndex < 0)
            return static_cast<int>(count) + rawIndex;
        return -1;
    }

    inline bool parseObjVertexKey(const std::string& token,
                                  const size_t positionCount,
                                  const size_t texcoordCount,
                                  const size_t normalCount,
                                  ObjVertexKey& keyOut)
    {
        keyOut = ObjVertexKey();

        const size_t firstSlash = token.find('/');
        const size_t secondSlash = firstSlash == std::string::npos ? std::string::npos : token.find('/', firstSlash + 1);

        const std::string positionToken = firstSlash == std::string::npos ? token : token.substr(0, firstSlash);
        const std::string texcoordToken = firstSlash == std::string::npos
            ? std::string()
            : token.substr(firstSlash + 1, (secondSlash == std::string::npos ? token.size() : secondSlash) - firstSlash - 1);
        const std::string normalToken = secondSlash == std::string::npos ? std::string() : token.substr(secondSlash + 1);

        if (positionToken.empty())
            return false;

        try
        {
            keyOut.positionIndex = resolveObjIndex(std::stoi(positionToken), positionCount);
            if (!texcoordToken.empty())
                keyOut.texcoordIndex = resolveObjIndex(std::stoi(texcoordToken), texcoordCount);
            if (!normalToken.empty())
                keyOut.normalIndex = resolveObjIndex(std::stoi(normalToken), normalCount);
        }
        catch (const std::exception&)
        {
            return false;
        }

        if (keyOut.positionIndex < 0 || keyOut.positionIndex >= static_cast<int>(positionCount))
            return false;
        if (keyOut.texcoordIndex >= static_cast<int>(texcoordCount))
            return false;
        if (keyOut.normalIndex >= static_cast<int>(normalCount))
            return false;

        return true;
    }

    inline std::string makeObjVertexKeyId(const ObjVertexKey& key)
    {
        return std::to_string(key.positionIndex) + "/" + std::to_string(key.texcoordIndex) + "/" + std::to_string(key.normalIndex);
    }


    static component::Mesh * LoadObj(string path)
    {
        std::ifstream fileStream(path, std::ios::in);
        if (!fileStream.is_open()) {
            throw "File could not be opened";
        }

        std::vector<glm::vec3> positions;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec2> texcoords;
        std::vector<ObjVertexKey> triangleVertices;
        triangleVertices.reserve(1024);

        bool sawAnyTexcoords = false;
        bool allReferencedNormalsAvailable = true;

        std::string line;
        while (std::getline(fileStream, line))
        {
            if (line.size() < 2 || line[0] == '#')
                continue;

            std::istringstream stream(line);
            std::string prefix;
            stream >> prefix;

            if (prefix == "v")
            {
                float x = 0.0f;
                float y = 0.0f;
                float z = 0.0f;
                if (stream >> x >> y >> z)
                    positions.push_back(glm::vec3(x, y, z));
            }
            else if (prefix == "vn")
            {
                float x = 0.0f;
                float y = 0.0f;
                float z = 0.0f;
                if (stream >> x >> y >> z)
                    normals.push_back(glm::vec3(x, y, z));
            }
            else if (prefix == "vt")
            {
                float u = 0.0f;
                float v = 0.0f;
                if (stream >> u >> v)
                {
                    texcoords.push_back(glm::vec2(u, 1.0f - v));
                    sawAnyTexcoords = true;
                }
            }
            else if (prefix == "f")
            {
                std::vector<ObjVertexKey> polygonVertices;
                std::string token;
                while (stream >> token)
                {
                    ObjVertexKey key;
                    if (!parseObjVertexKey(token, positions.size(), texcoords.size(), normals.size(), key))
                        continue;

                    if (key.texcoordIndex >= 0)
                        sawAnyTexcoords = true;
                    if (key.normalIndex < 0)
                        allReferencedNormalsAvailable = false;

                    polygonVertices.push_back(key);
                }

                if (polygonVertices.size() < 3)
                    continue;

                for (size_t index = 1; index + 1 < polygonVertices.size(); ++index)
                {
                    triangleVertices.push_back(polygonVertices[0]);
                    triangleVertices.push_back(polygonVertices[index]);
                    triangleVertices.push_back(polygonVertices[index + 1]);
                }
            }
        }

        std::unordered_map<std::string, uint> vertexLookup;
        std::vector<glm::vec3> meshPositions;
        std::vector<glm::vec3> meshNormals;
        std::vector<glm::vec2> meshUvs;
        std::vector<uint> meshTriangles;

        meshPositions.reserve(triangleVertices.size());
        meshNormals.reserve(triangleVertices.size());
        meshUvs.reserve(triangleVertices.size());
        meshTriangles.reserve(triangleVertices.size());

        for (const ObjVertexKey& key : triangleVertices)
        {
            const std::string keyId = makeObjVertexKeyId(key);
            const auto existing = vertexLookup.find(keyId);
            if (existing != vertexLookup.end())
            {
                meshTriangles.push_back(existing->second);
                continue;
            }

            const uint newIndex = static_cast<uint>(meshPositions.size());
            vertexLookup[keyId] = newIndex;
            meshTriangles.push_back(newIndex);

            meshPositions.push_back(positions[static_cast<size_t>(key.positionIndex)]);
            meshNormals.push_back(
                key.normalIndex >= 0 && key.normalIndex < static_cast<int>(normals.size())
                    ? normals[static_cast<size_t>(key.normalIndex)]
                    : glm::vec3(0.0f));
            meshUvs.push_back(
                key.texcoordIndex >= 0 && key.texcoordIndex < static_cast<int>(texcoords.size())
                    ? texcoords[static_cast<size_t>(key.texcoordIndex)]
                    : glm::vec2(0.0f));
        }

        const bool hasNormals = !normals.empty() && allReferencedNormalsAvailable;
        const bool hasUVs = sawAnyTexcoords;
        component::Mesh * mesh = new component::Mesh(
            static_cast<uint>(meshPositions.size()),
            static_cast<uint>(meshTriangles.size() / 3),
            hasNormals,
            false,
            hasUVs);

        for (size_t index = 0; index < meshPositions.size(); ++index)
        {
            mesh->setVertice(
                static_cast<uint>(index),
                meshPositions[index],
                hasNormals ? meshNormals[index] : glm::vec3(0.0f),
                glm::vec3(0.0f),
                hasUVs ? meshUvs[index] : glm::vec2(0.0f));
        }

        for (size_t index = 0; index < meshTriangles.size() / 3; ++index)
            mesh->setTriangle(static_cast<uint>(index), meshTriangles[index * 3], meshTriangles[index * 3 + 1], meshTriangles[index * 3 + 2]);

        if (!hasNormals)
            mesh->computeNormals();

        return mesh;
    }
    
    static component::Mesh * loadModelFile(std::string const & filename)
    {
        EXTENSION ext = getExtension(filename);

        switch (ext)
        {
            case OBJ:
                return LoadObj(filename);
            case OFF:
                return loadOFF(filename);
            default:
                throw "Invalid file format.";
        }
    }
}