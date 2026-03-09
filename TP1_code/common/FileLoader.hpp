#pragma once

#include <string>
#include <iostream>
#include <fstream>
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

    EXTENSION getExtension(std::string const & filename)
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
        vector<uint> tri = vector<uint>(n_faces * 3);

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
        component::Mesh * mesh = new Mesh(verts.size(), tri.size() / 3);
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

        vector<uint> tri = vector<uint>(n_faces * 3);

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

                tri[f*3] = _v1;
                tri[f*3+1] = _v2;
                tri[f*3+2] = _v3;
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
        component::Mesh * mesh = new Mesh(verts.size(), tri.size() / 3);
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





    float readFloat(uint & start, string str)
    {
        string buf = "";
        while (start < str.size())
            if ((str[start] == ' ' ||str[start] == '\r') && !buf.size() == 0)
                return atof(buf.c_str());
            else
                buf.push_back(str[start++]);
        return atof(buf.c_str());
    }
    uint readFUInt(uint & start, string & str)
    {
        string buf = "";
        while(start < str.size())
            if (str[start] == ' ' || str[start] == '/')
                return atoi(buf.c_str());
            else
                buf.push_back(str[start++]);
        return atoi(buf.c_str());
    }

    void readFace(uint & start, string str, uint & vert)
    {
        vert = readFUInt(start, str);
        if (start < str.size() && str[start] != ' ') // offset to next space, ignore potential tex + norm values
        {
            while (start < str.size() && str[start] != ' ')
                start++;
        }
    }

    bool __objLineQuad(uint start, string str)
    {
        uint spaceCount = 0;
        for (uint i = start; i < str.size() - 1; i++)
            if (str[i] == ' ' && str[i+1] != ' ')
                spaceCount++;
        return spaceCount == 4;
    }
    
    void readFace(uint & start, string & str, uint & vert, uint &tex, uint & norm)
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


    static component::Mesh * LoadObj(string path)
    {
        // 1. open file
        //   1.1. count elems
        // 2. construct temp buffers
        // 3. convert to internal mesh class
        // 4. return it.
                std::ifstream fileStream(path, std::ios::in);

        if (!fileStream.is_open()) {
            throw "File could not be opened";
        }

        uint vStride = 0;
        uint tStride = 0;
        uint nStride = 0;
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
                if (line[0] == 'v' && line[1] == 'n')
                    nStride++;
            }
        }
        fileStream.close();

        vector<float> verts = vector<float>(vStride * 3);
        vector<uint> tri = vector<uint>(tStride * 3);
        vector<float> norm = vector<float>(nStride * 3);
        vector<uint> normConv = vector<uint>(vStride * 3);
        std::ifstream fileStream2(path, std::ios::in);
        // now reread to actually collect and parse.
        if (!fileStream2.is_open()) {
            throw "File could not be opened";
        }
        uint v = 0;
        uint t = 0;
        uint n = 0;
        line = "";
        component::Mesh * mesh = new component::Mesh(vStride, tStride, nStride > 0);
        while (!fileStream2.eof())
        {
            getline(fileStream2, line);
            if (line[0] == 'v' && line[1] == ' ')
            {
                uint i = 2;
                verts[v*3] = readFloat(i, line);
                verts[v*3+1] = readFloat(++i, line);
                verts[v*3+2] = readFloat(++i, line);
                v++;
            }
            else if (line[0] == 'v' && line[1] == 'n')
            {
                uint i = 2;
                norm[n*3] = readFloat(i, line);
                norm[n*3+1] = readFloat(++i, line);
                norm[n*3+2] = readFloat(++i, line);
                n++;
            }
            else if (line[0] == 'f' && line[1] == ' ')
            {
                uint i = 2;
                uint t0, t1, t2;
                uint tu0, tu1, tu2;
                uint tn0, tn1, tn2;
                bool quad = __objLineQuad(i, line);
                readFace(i, line, t0, tu0, tn0);
                readFace(++i, line, t1, tu1, tn1);
                readFace(++i, line, t2, tu2, tn2);
                if (nStride > 0)
                {
                    normConv[t0 - 1] = tn0 - 1;
                    normConv[t1 - 1] = tn1 - 1;
                    normConv[t2 - 1] = tn2 - 1;
                }
                tri[t*3] = t0 - 1;
                tri[t*3+1] = t1 - 1;
                tri[t*3+2] = t2 - 1;
                t++;
                if (quad)
                {
                    uint t3, tu3, tn3;
                    readFace(++i, line, t3, tu3, tn3);
                    tri[t*3] = t0 - 1;
                    tri[t*3+1] = t2 - 1;
                    tri[t*3+2] = t3 - 1;
                    if (nStride > 0)
                        normConv[t3 - 1] = tn3 - 1;
                    t++;
                }
                
            }
        }
        fileStream2.close();
        for (uint i = 0; i < v; i++)
        {
            if (nStride == 0)
                mesh -> setVertice(i, glm::vec3(verts[i*3], verts[i*3+1], verts[i*3+2]));
            else
                mesh -> setVertice(i, glm::vec3(verts[i*3], verts[i*3+1], verts[i*3+2]), glm::vec3(norm[normConv[i]*3],norm[normConv[i]*3 + 1],norm[normConv[i*3]*3 + 2]));
        }
        for (uint i = 0; i < t; i++)
        {
            mesh -> setTriangle(i, tri[i*3], tri[i*3+1], tri[i*3+2]);
        }
        if (nStride == 0)
            mesh -> computeNormals();
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