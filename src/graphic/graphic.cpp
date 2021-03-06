#include "graphic.h"
#include <SDL2/SDL_image.h>

graphic::graphic( config *config) {
    p_index_light = 0;
    p_vao_quad = 0;

    // creating window
    p_display = new display( config);

    // loading the shader
    p_gbuffer = new Shader( "shader/g_buffer");
    p_deferred_shading = new Shader( "shader/deferred_shading");

    p_voxel = new Shader( "shader/voxels");
    p_object = new Shader( "shader/object");
    p_debug = new Shader( "shader/debug");

    // set up camera
    p_camera = new Camera( glm::vec3( -0.5f, 0.0f, -0.5f), graphic_fov, (float)p_display->getWidth()/(float)p_display->getHeight(), graphic_znear, graphic_zfar);

    initDeferredShading();

    // test lights
    /*createLight( glm::vec3( 6, 15, 0), glm::vec3( 0, 1.0, 1.0));
    createLight( glm::vec3( 0, 15, 6), glm::vec3( 1, 1.0, 1.0));
    createLight( glm::vec3( 6, 15, 6), glm::vec3( 1, 0.5, 0.5));*/
    createLight( glm::vec3( 0, -5, 0), glm::vec3( 1, 1, 1));
    createLight( glm::vec3( 0, 5, 0), glm::vec3( 1, 1, 1));
}

graphic::~graphic() {
    delete p_gbuffer;
    delete p_deferred_shading;
    delete p_camera;
    delete p_voxel;
    delete p_debug;
    delete p_display;
}

void graphic::resizeWindow( glm::vec2 screen) {
    glViewport(0, 0, screen.x, screen.y);
    printf( "graphic::resizeWindow resize screen to %.0fx%.0f\n", screen.x, screen.y);
    p_display->setSize( screen.x, screen.y);
    // resize
    p_camera->resize( (float)screen.x/(float)screen.y);

    resizeDeferredShading();
}

void graphic::initDeferredShading() {
    glm::vec2 l_scrn = p_display->getSize();

    glGenFramebuffers(1, &p_fbo_buffer);
    glBindFramebuffer(GL_FRAMEBUFFER, p_fbo_buffer);

    // 1 - position color buffer
    glGenTextures(1, &p_texture_position);
    glBindTexture(GL_TEXTURE_2D, p_texture_position);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, l_scrn.x, l_scrn.y, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, p_texture_position, 0);

    // 2 - normal color buffer
    glGenTextures(1, &p_texture_normal);
    glBindTexture(GL_TEXTURE_2D, p_texture_normal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, l_scrn.x, l_scrn.y, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, p_texture_normal, 0);

    // 3 - color + specular color buffer
    glGenTextures(1, &p_texture_colorSpec);
    glBindTexture(GL_TEXTURE_2D, p_texture_colorSpec);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, l_scrn.x, l_scrn.y, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, p_texture_colorSpec, 0);

    // 4 - normal color buffer
    glGenTextures(1, &p_texture_shadow);
    glBindTexture(GL_TEXTURE_2D, p_texture_shadow);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, l_scrn.x, l_scrn.y, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, p_texture_shadow, 0);

    // 4 - tell OpenGL which color attachments we'll use (of this framebuffer) for rendering
    unsigned int l_attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
    glDrawBuffers( 4, l_attachments);

    // 5 - depth buffer
    glGenRenderbuffers(1, &p_depth);
    glBindRenderbuffer(GL_RENDERBUFFER, p_depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, l_scrn.x, l_scrn.y);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, p_depth);

    // finally check if framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        printf( "graphic::initDeferredShading framebuffer didnt complete!\n");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    p_deferred_shading->Bind();
    p_deferred_shading->setInt("gPosition", 0);
    p_deferred_shading->setInt("gNormal", 1);
    p_deferred_shading->setInt("gAlbedoSpec", 2);
    p_deferred_shading->setInt("gShadow", 3);
}

void graphic::resizeDeferredShading() {
    glm::vec2 l_scrn = p_display->getSize();

    glBindFramebuffer(GL_FRAMEBUFFER, p_fbo_buffer);

    glBindTexture(GL_TEXTURE_2D, p_texture_position);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, l_scrn.x, l_scrn.y, 0, GL_RGB, GL_FLOAT, NULL);

    glBindTexture(GL_TEXTURE_2D, p_texture_normal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, l_scrn.x, l_scrn.y, 0, GL_RGB, GL_FLOAT, NULL);

    glBindTexture(GL_TEXTURE_2D, p_texture_colorSpec);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, l_scrn.x, l_scrn.y, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

    glBindTexture(GL_TEXTURE_2D, p_texture_shadow);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, l_scrn.x, l_scrn.y, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

    glBindRenderbuffer(GL_RENDERBUFFER, p_depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, l_scrn.x, l_scrn.y);
}

void graphic::renderDeferredShading() {
    glm::vec2 l_scrn = p_display->getSize();

    p_deferred_shading->Bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, p_texture_position);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, p_texture_normal);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, p_texture_colorSpec);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, p_texture_shadow);
    // send light relevant uniforms
    for (unsigned int i = 0; i < p_lights.size(); i++)
    {
        p_deferred_shading->setVec3("lights[" + std::to_string(i) + "].Position", p_lights[i].getPos());
        p_deferred_shading->setVec3("lights[" + std::to_string(i) + "].Color", p_lights[i].getColor());

        // update attenuation parameters and calculate radius
        float constant  = 10.0;
        float linear    = 1.5;
        float quadratic = 1.1;
        p_deferred_shading->setFloat("lights[" + std::to_string(i) + "].Linear", linear);
        p_deferred_shading->setFloat("lights[" + std::to_string(i) + "].Quadratic", quadratic);
        // then calculate radius of light volume/sphere
        const float maxBrightness = std::fmaxf( std::fmaxf(p_lights[i].getColor().r, p_lights[i].getColor().g), p_lights[i].getColor().b);
        float radius = (-linear + std::sqrt(linear * linear - 4 * quadratic * (constant - (256.0f / 5.0f) * maxBrightness))) / (2.0f * quadratic);
        p_deferred_shading->setFloat("lights[" + std::to_string(i) + "].Radius", radius);
    }

    // render
    renderQuad();

    glBindFramebuffer(GL_READ_FRAMEBUFFER, p_fbo_buffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // write to default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, l_scrn.x, l_scrn.y, 0, 0, l_scrn.x, l_scrn.y, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
}

void graphic::renderQuad()
{
    // if dont setup -> creating vao
    if (p_vao_quad == 0)
    {
        float l_vertices_and_texture[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };

        // setup vao
        glGenVertexArrays(1, &p_vao_quad);
        glGenBuffers(1, &p_vbo_quad);
        glBindVertexArray(p_vao_quad);

        // upload data once
        glBindBuffer(GL_ARRAY_BUFFER, p_vbo_quad);
        glBufferData(GL_ARRAY_BUFFER, sizeof(l_vertices_and_texture), &l_vertices_and_texture, GL_STATIC_DRAW);

        // setup vbo
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    // render vao ( rect)
    glBindVertexArray(p_vao_quad);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void graphic::bindDeferredShading() {
    // render scene in geometry/color data
    getDisplay()->clear( true);

    glBindFramebuffer( GL_FRAMEBUFFER, p_fbo_buffer);
    getDisplay()->clear( false);
}


SDL_Surface* graphic::loadSurface(std::string File) {
    // Laden der Datei
    SDL_Surface* loaded = IMG_Load(File.c_str());
	if( loaded == NULL ) {
        printf( "graphic::loadSurface unable to load image %s!", File.c_str() ); // Fehler
        printf( "%s", SDL_GetError() );
        return NULL;
    } else {
        // Color key ferstlegen
        //SDL_SetColorKey( loaded, true, SDL_MapRGB( loaded->format, 200, 191, 231));
    }
    return loaded;
}

void graphic::draw( SDL_Surface* Image, double X, double Y, int W, int H, int SpriteX, int SpriteY, bool Flip) {
    SDL_Rect destination;
    destination.x = (int)X;
    destination.y = (int)Y;
    destination.w = W;
    destination.h = H;
    SDL_Rect source;
    source.x = SpriteX*W;
    source.y = SpriteY*H;
    source.w = W;
    source.h = H;
    if( Image == NULL )
        printf( "graphic::draw image dont load...\n");
    if( p_display->getSurface() == NULL )
        printf( "graphic::draw screen dont load...\n");
    SDL_BlitSurface( Image, &source, p_display->getSurface(), &destination);
}

void graphic::saveImageBMP( std::string File) {
    // no save if no graphics
    #ifdef NO_GRAPHICS
    return;
    #endif // NO_GRAPHICS

    // Save Image
    File = File + ".png";
    // Image save
    if( IMG_SavePNG( p_display->getSurface(), File.c_str())) {
        printf("Unable to save png -- %s\n", SDL_GetError());
    }
}

int graphic::createLight( glm::vec3 pos, glm::vec3 color) {
    light l_light( p_index_light);

    l_light.setPositon( pos);
    l_light.setColor( color);

    p_index_light++;

    p_lights.push_back( l_light);
    return p_index_light-1;
}
