#include "Engine.h"

#include "TextureCompressor.h"


int main() 
{
    TextureCompressor::CompressTextures("rawtextures", "textures");

    UEngine Engine;
    Engine.Run();

    return 0;
}