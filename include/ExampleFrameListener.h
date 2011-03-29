/*
 -----------------------------------------------------------------------------
 This source file is part of OGRE
 (Object-oriented Graphics Rendering Engine)
 For the latest info, see http://www.ogre3d.org/

 Copyright (c) 2000-2006 Torus Knot Software Ltd
 Also see acknowledgements in Readme.html

 You may use this sample code for anything you like, it is not covered by the
 LGPL like the rest of the engine.
 -----------------------------------------------------------------------------
 */
/*
 -----------------------------------------------------------------------------
 Filename:    ExampleFrameListener.h
 Description: Defines an example frame listener which responds to frame events.
 This frame listener just moves a specified camera around based on
 keyboard and mouse movements.
 -----------------------------------------------------------------------------
 */

#ifndef __ExampleFrameListener_H__
#define __ExampleFrameListener_H__

#include <cstdlib>
#include <pthread.h>

#include "OgreFrameListener.h"
#include "OgreStringConverter.h"
#include "OgreException.h"

#include <deque>
#include <CEGUI.h>
#include <OIS.h>
#include <CEGUIRenderer.h>

//Use this define to signify OIS will be used as a DLL
//(so that dll import/export macros are in effect)
#define OIS_DYNAMIC_LIB
#include <OIS.h>

#include "TextRenderer.h"
#include "Socket.h"
#include "Tile.h"
#include "ChatMessage.h"
#include "Room.h"
#include "MusicPlayer.h"
#include "RenderManager.h"


class SoundEffectsHelper;

/*! \brief The main OGRE rendering class.
 *
 * This class provides the rendering framework for the OGRE subsystem, as well
 * as processing keyboard and mouse input from the user.  It loads and
 * initializes the meshes for creatures and tiles, moves the camera, and
 * displays the terminal and chat messages on the game screen.
 */
class ExampleFrameListener: public Ogre::FrameListener,
        public Ogre::WindowEventListener,
        public OIS::MouseListener,
        public OIS::KeyListener
{
    protected:
        void updateStats(void);

    public:
        // Constructor takes a RenderWindow because it uses that to determine input context
        ExampleFrameListener(Ogre::RenderWindow* win, Ogre::Camera* cam,
                Ogre::SceneManager *sceneManager, CEGUI::Renderer *renderer,
                bool bufferedKeys, bool bufferedMouse, bool bufferedJoy);

        //Adjust mouse clipping area
        virtual void windowResized(Ogre::RenderWindow* rw);

        //Unattach OIS before window shutdown (very important under Linux)
        virtual void windowClosed(Ogre::RenderWindow* rw);
        virtual ~ExampleFrameListener();

		void moveCamera(Ogre::Real frameTime);
        Ogre::Vector3 getCameraViewTarget();
        void flyTo(Ogre::Vector3 destination);

        void showDebugOverlay(bool show);

        // Override frameStarted event to process that (don't care about frameEnded)
        bool frameStarted(const Ogre::FrameEvent& evt);
        bool frameEnded(const Ogre::FrameEvent& evt);

        //CEGUI Functions
        bool quit(const CEGUI::EventArgs &e);
        bool mouseMoved(const OIS::MouseEvent &arg);
        Ogre::RaySceneQueryResult& doRaySceneQuery(const OIS::MouseEvent &arg);
        bool mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id);
        bool mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id);
        bool keyPressed(const OIS::KeyEvent &arg);
        bool keyReleased(const OIS::KeyEvent &arg);
        void handleHotkeys(int hotkeyNumber);

        // Console functions
        void printText(string text);
        void executePromptCommand(string command, string arguments);
        string getHelpText(string arg);

        // Console variables
        string command, arguments, commandOutput, prompt;
        //deque< pair<time_t, string> > chatMessages;
        std::deque<ChatMessage*> chatMessages;
        string consoleBuffer, promptCommand, chatString;

        // Multiplayer stuff
        std::vector<Socket*> clientSockets;
        pthread_t clientThread;
        pthread_t serverThread;
        pthread_t serverNotificationThread;
        pthread_t clientNotificationThread;
        std::vector<pthread_t*> clientHandlerThreads;
        pthread_t creatureThread;

        // Variables for chat messages
        unsigned int chatMaxMessages;
        unsigned int chatMaxTimeDisplay;

        bool mContinue;

    protected:
        Ogre::Camera* mCamera;
        Ogre::SceneNode *mCamNode;

        Ogre::Vector3 translateVector;
        Ogre::Vector3 translateVectorAccel;
        Ogre::Vector3 mMouseTranslateVector;
        Ogre::Vector3 cameraFlightDestination;
        Ogre::Vector3 mRotateLocalVector;
        double zChange;
        //Ogre::Vector3 mRotateWorldVector;
        Ogre::RenderWindow* mWindow;
        bool cameraIsFlying;
        Ogre::Real moveSpeed;
        Ogre::Real moveSpeedAccel;
        Ogre::Degree mRotateSpeed;
        Ogre::Degree swivelDegrees;
		Ogre::Real cameraFlightSpeed;
        bool hotkeyLocationIsValid[10];
        Ogre::Vector3 hotkeyLocation[10];

        std::string mDebugText;
        bool mStatsOn;

        unsigned int mNumScreenShots;
        float mMoveScale;
        float mZoomSpeed;
        Ogre::Degree mRotScale;
        // just to stop toggles flipping too fast
        Ogre::Real mTimeUntilNextToggle;
        Ogre::Radian mRotX, mRotY, mRotZ;
        Ogre::TextureFilterOptions mFiltering;
        int mAniso;
        Tile::TileType mCurrentTileType;
        int mCurrentFullness, mCurrentTileRadius;
        bool mBrushMode;
        bool addRoomsMode;
        double frameDelay;

        int mSceneDetailIndex;
        Ogre::Overlay* mDebugOverlay;

        //OIS Input devices
        OIS::InputManager* mInputManager;
        OIS::Mouse* mMouse;
        OIS::Keyboard* mKeyboard;
        OIS::JoyStick* mJoy;

        // Mouse query stuff
        Ogre::RaySceneQuery *mRaySceneQuery; // The ray scene query pointer
        bool mLMouseDown, mRMouseDown; // True if the mouse buttons are down
        int mLStartDragX, mLStartDragY; // The start tile coordinates for a left drag
        int mRStartDragX, mRStartDragY; // The start tile coordinates for a left drag
        int mCount; // The number of robots on the screen
        Ogre::SceneManager *mSceneMgr; // A pointer to the scene manager
        Ogre::SceneNode *mCurrentObject; // The newly created object
        CEGUI::Renderer *mGUIRenderer; // CEGUI renderer
        int xPos, yPos;
        bool digSetBool; // For server mode - hods whether to mark or unmark a tile for digging
        bool mouseDownOnCEGUIWindow;

        enum DragType
        {
            creature,
            mapLight,
            tileSelection,
            tileBrushSelection,
            addNewRoom,
            addNewTrap,
            nullDragType
        };

        //OgreOggSound::OgreOggISound* mCurrMusic;

        RenderManager* renderManager;

    private:
        bool terminalActive;
        int terminalWordWrap;

        DragType mDragType;
        string draggedCreature, draggedMapLight;
        Ogre::SceneNode *creatureSceneNode, *roomSceneNode, *fieldSceneNode,
                *lightSceneNode;

        SoundEffectsHelper* sfxHelper;
        MusicPlayer* musicPlayer;
};

#endif
