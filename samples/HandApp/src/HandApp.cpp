/*
* 
* Copyright (c) 2013, Ban the Rewind
* All rights reserved.
* 
* Redistribution and use in source and binary forms, with or 
* without modification, are permitted provided that the following 
* conditions are met:
* 
* Redistributions of source code must retain the above copyright 
* notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright 
* notice, this list of conditions and the following disclaimer in 
* the documentation and/or other materials provided with the 
* distribution.
* 
* Neither the name of the Ban the Rewind nor the names of its 
* contributors may be used to endorse or promote products 
* derived from this software without specific prior written 
* permission.
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
* FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
* COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
* BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
* ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
* 
*/

#include "cinder/app/AppBasic.h"
#include "cinder/Camera.h"
#include "Cinder-OpenNI.h"
#include "Emitter.h"
#include "Particle.h"

/* 
* This application demonstrates how to display NiTE hands.
*/
class HandApp : public ci::app::AppBasic 
{

public:
	void					update();
	void					draw();
	void					keyDown( ci::app::KeyEvent event );
	void					prepareSettings( ci::app::AppBasic::Settings* settings );
	void					setup();
private:
	OpenNI::DeviceManager	mDeviceManager;
	OpenNI::DeviceRef		mDevice;
	ci::Channel16u			mChannel;
	void					onHand( nite::HandTrackerFrameRef frame );

	void					screenShot();

	//std::vector< nite::HandData >		mTrackedHands;
	std::map< nite::HandId, EmitterRef >	mEmitters;
	std::vector< ParticleRef >				mParticles;

	ci::CameraPersp			mCamera;
};

#include "cinder/gl/Texture.h"
#include "cinder/ImageIo.h"
#include "cinder/Utilities.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace OpenNI;

void HandApp::update()
{
	
}

void HandApp::draw()
{
	gl::setViewport( getWindowBounds() );
	gl::clear( Colorf::black() );

	// draw depth stream in 2d
	gl::setMatricesWindow( getWindowSize() );

	if ( mChannel )
	{
		gl::color( Colorf::white() );
		gl::TextureRef tex = gl::Texture::create( Channel8u( mChannel ) );
		gl::draw( tex, tex->getBounds(), getWindowBounds() );
	}

	// draw 3d content
	gl::setMatrices( mCamera );
	gl::enableWireframe();

	// draw emitters
	for ( const auto& emitterPair : mEmitters )
	{
		( emitterPair.first % 2 ) ? gl::color( 1.0f, 0.0f, 0.0f ) : gl::color( 0.0f, 0.0f, 1.0f );
		gl::drawSphere( emitterPair.second->getPosition(), 25.0f );
	}

	gl::disableWireframe();
}

void HandApp::keyDown( KeyEvent event )
{
	switch ( event.getCode() ) {
	case KeyEvent::KEY_q:
		quit();
		break;
	case KeyEvent::KEY_f:
		setFullScreen( !isFullScreen() );
		break;
	case KeyEvent::KEY_s:
		screenShot();
		break;
	}
}

void HandApp::onHand( nite::HandTrackerFrameRef frame )
{
	openni::VideoFrameRef depthFrame	= frame.getDepthFrame();
	mChannel							= toChannel16u( depthFrame );

	std::vector< nite::GestureData >	gestures		= toVector( frame.getGestures() );

	for ( auto iter = gestures.begin(); iter != gestures.end(); ++iter )
	{
		if ( iter->isComplete() )
		{
			console() << "Gesture Completed: ";

			if ( iter->getType() == nite::GestureType::GESTURE_WAVE )
			{
				console() << "WAVE" << endl;

				nite::HandId id;
				mDevice->getHandTracker().startHandTracking( iter->getCurrentPosition(), &id );
			}
			else if ( iter->getType() == nite::GestureType::GESTURE_CLICK )
			{
				console() << "CLICK" << endl;
			}
			else if ( iter->getType() == nite::GestureType::GESTURE_HAND_RAISE )
			{
				console() << "HAND RAISE" << endl;
			}
		}
	}

	auto hands	= toVector( frame.getHands() );

	for ( const auto& hand : hands )
	{
		if ( hand.isTracking() && !hand.isLost() )
		{
			Vec3f position		= toVec3f( hand.getPosition() );
			position.x			= -position.x;

			if ( hand.isNew() )
			{
				mEmitters.insert( std::make_pair( hand.getId(), Emitter::create( position ) ) );
			}
			else
			{
				mEmitters.at( hand.getId() )->update( position );
			}
		}
		else
		{
			mEmitters.erase( hand.getId() );
		}
	}
}

void HandApp::prepareSettings( Settings* settings )
{
	//settings->enableConsoleWindow();
	settings->setFrameRate( 60.0f );
	settings->setWindowSize( 800, 600 );
}

void HandApp::screenShot()
{
	writeImage( getAppPath() / fs::path( "frame" + toString( getElapsedFrames() ) + ".png" ), copyWindowSurface() );
}

void HandApp::setup()
{
	Vec2i windowSize = toPixels( getWindowSize() );
	mCamera = CameraPersp( windowSize.x, windowSize.y, 45.0f, 0.1f, 10000.0f );
	mCamera.lookAt( Vec3f::zero(), Vec3f::zAxis(), Vec3f::yAxis() );

	try {
		mDevice = mDeviceManager.createDevice( OpenNI::DeviceOptions().enableHandTracking() );
	} catch ( ExcDeviceNotFound ex ) {
		console() << ex.what() << endl;
		quit();
	} catch ( ExcDeviceNotAvailable ex ) {
		console() << ex.what() << endl;
		quit();
	}
	
	mDevice->connectHandEventHandler( &HandApp::onHand, this );
	mDevice->start();
	//mDevice->getHandTracker().startGestureDetection( nite::GestureType::GESTURE_CLICK );
	mDevice->getHandTracker().startGestureDetection( nite::GestureType::GESTURE_WAVE );
}

CINDER_APP_BASIC( HandApp, RendererGl )
