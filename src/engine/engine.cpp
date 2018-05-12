#include "engine.h"
#include "../graphic/glerror.h"
#include "../system/timer.h"
#include <stdio.h>

std::string NumberToString( double Number) {
    /*std::ostringstream ss;
    ss << Number;
    return ss.str();*/
    char buffer[255];
    sprintf( buffer, "%0.2f", Number);
    std::string temp = buffer;
    return temp;
}

engine::engine() {
    // set values
    p_openvr = NULL;
    p_isRunnig = true;
    p_framecap = true;
    p_world_player = NULL;
    p_timecap = 12; // ms -> 90hz

    p_config = new config();
    p_graphic = new graphic( p_config);

    // set block list up
    p_blocklist = new block_list( p_config);
    p_blocklist->init( p_graphic, p_config);

    // set up start world
    world *l_world = new world( p_blocklist);
    int l_size = 2;
    int l_end = -2;
    for( int x = -l_size; x <= l_size; x++)
        for( int z = -l_size; z <= l_size; z++)
            for( int y = 1; y > l_end; y--)
                l_world->addChunk( glm::vec3( x, y, z), true);
    p_worlds.push_back( l_world);

    p_world_player = p_worlds[0];

    //p_network = new network( p_config, p_tilemap, p_blocklist);
}

engine::~engine() {
    for( auto l_world:p_worlds)
        delete l_world;
    p_worlds.clear();
    if( p_openvr)
        delete p_openvr;
    delete p_blocklist;
    delete p_graphic;
    //delete p_network;
    delete p_config;
}

void engine::startVR() {
    p_openvr = new openvr();
    p_framecap = false;
}

void engine::raycastView( glm::vec3 position, glm::vec3 lookat, int forward) {
    glm::vec3 l_postion_ray = position;
    glm::vec3 l_postion_prev = position;
    glm::vec3 l_block = { 0, 0, 0};
    glm::vec3 l_block_prev;
    bool l_found = false;

    if( p_world_player == NULL)
        return;

    for(int i = 0; i < forward; i++) {
        l_postion_prev = l_postion_ray;
        l_postion_ray += lookat * 0.1f;

        l_block.x = floorf( l_postion_ray.x);
        l_block.y = floorf( l_postion_ray.y);
        l_block.z = floorf( l_postion_ray.z);

        Chunk *l_chunk = p_world_player->getChunkWithPosition( l_block);
        if( l_chunk) {
            glm::vec3 l_chunk_pos = l_chunk->getPos() * glm::ivec3( CHUNK_SIZE);
            if( l_chunk->getTile( l_block - l_chunk_pos) != EMPTY_BLOCK_ID) { // check for block
                l_found = true;
                break;
            }
        }
    }
    if( !l_found)
        return;

    // find out witch face we looking
    glm::vec3 l_postion_floor_prev;
    l_postion_floor_prev.x = floorf( l_postion_prev.x);
    l_postion_floor_prev.y = floorf( l_postion_prev.y);
    l_postion_floor_prev.z = floorf( l_postion_prev.z);

    l_block_prev = l_block;

    if( l_postion_floor_prev.x > l_block.x)
        l_block_prev.x++;
    else if( l_postion_floor_prev.x < l_block.x)
        l_block_prev.x--;
    else if( l_postion_floor_prev.y > l_block.y)
        l_block_prev.y++;
    else if( l_postion_floor_prev.y < l_block.y)
        l_block_prev.y--;
    else if( l_postion_floor_prev.z > l_block.z)
        l_block_prev.z++;
    else if( l_postion_floor_prev.z < l_block.z)
        l_block_prev.z--;

    // input handling
    if( p_input.Map.Place && !p_input.MapOld.Place) {
        Chunk *l_chunk = p_world_player->getChunkWithPosition( l_block_prev);
        if( l_chunk) {
            p_world_player->changeBlock( l_chunk, l_block_prev, p_blocklist->getByName( "glowcrystal")->getID());
            //p_world_player->addTorchlight( l_chunk, l_block_prev, LIGHTING_MAX);
        }
    }

    if( p_input.Map.Destory && !p_input.MapOld.Destory) {
        Chunk *l_chunk = p_world_player->getChunkWithPosition( l_block);
        if( l_chunk)
            p_world_player->changeBlock( l_chunk, l_block, EMPTY_BLOCK_ID);
    }
}

void engine::render( glm::mat4 view, glm::mat4 projection) {
    Shader *l_shader = NULL;

    // biding deferred shading
    p_graphic->bindDeferredShading();
    p_graphic->getDisplay()->clear( false);

    // render voxel
    l_shader = p_graphic->getVoxelShader();
    l_shader->Bind();
    l_shader->update( MAT_PROJECTION, projection);
    l_shader->update( MAT_VIEW, view);
    p_world_player->draw( p_graphic, l_shader);

    // render object
    /*l_shader = p_graphic->getObjectShader();
    l_shader->Bind();
    l_shader->update( MAT_PROJECTION, projection);
    l_shader->update( MAT_VIEW, view);
    p_graphic->addShadowMatrix( l_shader);
    p_network->drawEntitys( l_shader);*/

    // we are done
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void engine::fly( int l_delta) {
    float Speed = 0.005f;
    Camera *cam = p_graphic->getCamera();
    if( p_input.Map.Up )
        cam->MoveForwardCross( Speed*l_delta);
    if( p_input.Map.Down )
        cam->MoveForwardCross(-Speed*l_delta);
    if( p_input.Map.Right )
        cam->MoveRight(-Speed*l_delta);
    if( p_input.Map.Left )
        cam->MoveRight( Speed*l_delta);
    if( p_input.Map.Jump )
        cam->MoveUp( Speed*l_delta);
    if( p_input.Map.Shift )
        cam->MoveUp( -Speed*l_delta);
}

void engine::run() {
    // set variables
    Timer l_timer;
    std::string l_title;
    struct clock l_clock;
    glm::mat4 l_mvp;

    int l_delta = 0;

    Timer l_timer_test;
    /*ServerCreated_ServerSerialized* l_hand = NULL;

    if( p_network->isServer()) {
        l_hand = new ServerCreated_ServerSerialized();
        l_hand->setPosition( glm::vec3( 0, 15, 5) );
        p_network->addObject( l_hand);
        l_hand->p_name = "box";
    }*/

    //l_hand->p_type = p_network->getObjectList()->get( l_hand->p_name.C_String());

    if( p_openvr) { // dirty hack
        glm::vec2 l_oldSize = p_graphic->getDisplay()->getSize();
        p_graphic->getDisplay()->setSize( p_openvr->getScreenSize());
        p_graphic->resizeDeferredShading();
    }

    p_graphic->getCamera()->GetPos().y = 20;

    // set up clock
    l_clock.tick();
    while( p_isRunnig) { // Runniz
        l_timer.Start();

        p_input.Reset();
        p_isRunnig = p_input.Handle( p_graphic->getWidth(), p_graphic->getHeight(), p_graphic->getWindow());

        Camera *cam = p_graphic->getCamera();
        cam->horizontalAngle ( -p_input.Map.MousePos.x * 2);
        cam->verticalAngle  ( p_input.Map.MousePos.y * 2);

        /// process
        if( p_config->get( "fly", "engine", "false") == "true")
            fly( l_delta);

        if( p_input.getResize()) {
            p_graphic->resizeWindow( p_input.getResizeW(), p_input.getResizeH());
            p_config->set( "width", std::to_string( p_input.getResizeW()), "graphic");
            p_config->set( "height", std::to_string( p_input.getResizeH()), "graphic");
        }

        //if( p_network->process( l_delta))
        //    p_isRunnig = false;

        if( p_input.Map.Inventory && !p_input.MapOld.Inventory ) {
            //if( l_hand)
            //    l_hand->setPosition( cam->GetPos() + glm::vec3( 0, 2, 0) );
            //l_hand->getBody()->SetLinearVelocity( q3Vec3( 0, 0, 0));
            //l_hand->getBody()->ApplyForceAtWorldPoint( q3Vec3( 0, -100, 100),  q3Vec3( 0, 0, 0));
        }

        /// render #1 openVR
        if( p_openvr ) {
            glm::mat4 l_projection = p_openvr->getCurrentProjectionMatrix( vr::Eye_Left);
            glm::mat4 l_view_cam =  p_openvr->getCurrentViewMatrix( vr::Eye_Left) * p_graphic->getCamera()->getViewWithoutUp();
            render( l_view_cam, l_projection);
            p_openvr->renderForLeftEye();
            p_graphic->renderDeferredShading();
            p_openvr->renderEndLeftEye();

            l_projection = p_openvr->getCurrentProjectionMatrix( vr::Eye_Right);
            l_view_cam = p_openvr->getCurrentViewMatrix( vr::Eye_Right) * p_graphic->getCamera()->getViewWithoutUp();
            render( l_view_cam, l_projection);
            p_openvr->renderForRightEye();
            p_graphic->renderDeferredShading();
            p_openvr->renderEndRightEye();

            //l_timer.Start();
            p_openvr->renderFrame();
        }


        /// render #2 window


        glm::mat4 l_view_cam = p_graphic->getCamera()->getView();
        glm::mat4 l_projection = p_graphic->getCamera()->getProjection();

        l_timer_test.Start();
        render( l_view_cam, l_projection);
        p_graphic->getDisplay()->clear( false);
        p_graphic->renderDeferredShading();

        raycastView( p_graphic->getCamera()->GetPos(), p_graphic->getCamera()->GetForward(), 20);

        //viewCurrentBlock( l_projection * l_view_cam, 275); // 275 = 2,75Meter

        p_graphic->getDisplay()->swapBuffers();

        /// calculation  time
        // print opengl error number
        GLenum l_error =  glGetError();
        if( l_error) {
            std::cout << "engine::run OpenGL error #" << l_error << std::endl;
        }

        // framerate
        p_framerate.push_back( l_clock.delta);
        if( p_framerate.size() > 10)
            p_framerate.erase( p_framerate.begin());
        float l_average_delta_time = 0;
        for( int i = 0; i < (int)p_framerate.size(); i++)
            l_average_delta_time += (float)p_framerate[i];
        l_average_delta_time = l_average_delta_time/(float)p_framerate.size();

        double averageFrameTimeMilliseconds = 1000.0/(l_average_delta_time==0?0.001:l_average_delta_time);
        l_title = "FPS_" + NumberToString( averageFrameTimeMilliseconds );
        l_title = l_title + " " + NumberToString( (double)l_timer.GetTicks()) + "ms";
        l_title = l_title + " X_" + NumberToString( cam->GetPos().x) + " Y_" + NumberToString( cam->GetPos().y) + " Z_" + NumberToString( cam->GetPos().z );
        l_title = l_title + " Chunks_" + NumberToString( (double) p_world_player->getAmountChunks()) + "/" + NumberToString( (double)p_world_player->getAmountChunksVisible() );
        p_graphic->getDisplay()->setTitle( l_title);

        // one at evry frame
        l_clock.tick();

        // frame rate
        if( l_clock.delta < p_timecap && p_framecap) {
            int l_sleep = p_timecap - l_clock.delta;
            SDL_Delay( l_sleep==0?1:l_sleep);

        }

        l_delta = l_timer.GetTicks();
    }
}

/*void engine::drawBox( glm::mat4 viewProjection, glm::vec3 pos) {
    std::vector<block_data> t_box;

    // Chunk Vbo Data Struct
    t_box.resize( 24 );

    // x-y side
    t_box[0] = block_data(0, 0, 0, 14);
    t_box[1] = block_data(1, 0, 0, 14);
    t_box[2] = block_data(0, 0, 0, 14);
    t_box[3] = block_data(0, 1, 0, 14);
    t_box[4] = block_data(1, 0, 0, 14);
    t_box[5] = block_data(1, 1, 0, 14);
    t_box[6] = block_data(1, 1, 0, 14);
    t_box[7] = block_data(0, 1, 0, 14);
    // x-y & z+1
    t_box[8] = block_data(0, 0, 1, 14);
    t_box[9] = block_data(1, 0, 1, 14);
    t_box[10] = block_data(0, 0, 1, 14);
    t_box[11] = block_data(0, 1, 1, 14);
    t_box[12] = block_data(1, 0, 1, 14);
    t_box[13] = block_data(1, 1, 1, 14);
    t_box[14] = block_data(1, 1, 1, 14);
    t_box[15] = block_data(0, 1, 1, 14);
    // restlichen linien
    t_box[16] = block_data(0, 0, 0, 14);
    t_box[17] = block_data(0, 0, 1, 14);
    t_box[18] = block_data(0, 1, 0, 14);
    t_box[19] = block_data(0, 1, 1, 14);
    t_box[20] = block_data(1, 0, 0, 14);
    t_box[21] = block_data(1, 0, 1, 14);
    t_box[22] = block_data(1, 1, 0, 14);
    t_box[23] = block_data(1, 1, 1, 14);

    Transform f_form;
    f_form.setPos( pos);
    f_form.setScale( glm::vec3( CHUNK_SCALE) );

    if( p_vboCursor == NULL)
        glGenBuffers(1, &p_vboCursor);


    // Shader einstellen
    p_graphic->getVertexShader()->BindArray( p_vboCursor, 0);
    p_graphic->getVertexShader()->Bind();// Shader
    p_graphic->getVertexShader()->EnableVertexArray( 0);
    p_graphic->getVertexShader()->update( f_form.getModel(), viewProjection);

    // Vbo übertragen
    glBindBuffer(GL_ARRAY_BUFFER, p_vboCursor);
    glBufferData(GL_ARRAY_BUFFER, t_box.size() * sizeof(block_data), &t_box[0], GL_DYNAMIC_DRAW);

    // Zeichnen
    glDrawArrays( GL_LINES, 0, 24);
}*/
