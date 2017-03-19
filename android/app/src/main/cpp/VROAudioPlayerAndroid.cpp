//
//  VROAudioPlayerAndroid.cpp
//  ViroRenderer
//
//  Created by Raj Advani on 11/17/16.
//  Copyright © 2016 Viro Media. All rights reserved.
//

#include "VROAudioPlayerAndroid.h"

VROAudioPlayerAndroid::VROAudioPlayerAndroid(std::string fileName) :
    _fileName(fileName) {
    _player = new VROAVPlayer();
}

VROAudioPlayerAndroid::VROAudioPlayerAndroid(std::shared_ptr<VROSoundData> data) {
    _player = new VROAVPlayer();
    _data = data;
}

VROAudioPlayerAndroid::~VROAudioPlayerAndroid() {
    delete (_player);
}

void VROAudioPlayerAndroid::setLoop(bool loop) {
    _player->setLoop(loop);
}

void VROAudioPlayerAndroid::play() {
    _player->play();
}

void VROAudioPlayerAndroid::pause() {
    _player->pause();
}

void VROAudioPlayerAndroid::setVolume(float volume) {
    _player->setVolume(volume);
}

void VROAudioPlayerAndroid::setMuted(bool muted) {
    _player->setMuted(muted);
}

void VROAudioPlayerAndroid::seekToTime(float seconds) {
    // this is just a generic duration from the Android MediaPlayer
    int totalDuration = _player->getVideoDurationInSeconds();
    if (seconds > totalDuration) {
        seconds = totalDuration;
    } else if (seconds < 0) {
        seconds = 0;
    }
    _player->seekToTime(seconds);
}

void VROAudioPlayerAndroid::setDelegate(std::shared_ptr<VROSoundDelegateInternal> delegate) {
    _delegate = delegate;
    std::shared_ptr<VROAVPlayerDelegate> avDelegate =
            std::dynamic_pointer_cast<VROAVPlayerDelegate>(shared_from_this());
    _player->setDelegate(avDelegate);
}

void VROAudioPlayerAndroid::setup() {
    if (_data) {
        _data->setDelegate(shared_from_this());
    }
    if (!_fileName.empty()) {
        _player->setDataSourceURL(_fileName.c_str());
    }
}

#pragma mark - VROAVPlayerDelegate

void VROAudioPlayerAndroid::onPrepared() {
    if (_delegate) {
        _delegate->soundIsReady();
    }
}

void VROAudioPlayerAndroid::onFinished() {
    if (_delegate) {
        _delegate->soundDidFinish();
    }
}

void VROAudioPlayerAndroid::onError(std::string error) {
    if (_delegate) {
        // TODO VIRO-902 Error callbacks for sound
    }
}

#pragma mark - VROSoundDataDelegate

void VROAudioPlayerAndroid::dataIsReady() {
    if (_player) {
        _player->setDataSourceURL(_data->getLocalFilePath().c_str());
    }
}

void VROAudioPlayerAndroid::dataError() {
    // TODO VIRO-902 bubble data loading errors up to JS
}