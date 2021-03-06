#include "MusicPlayerList.h"

#include <coreutils/log.h>
#include <coreutils/utils.h>
#include <algorithm>
#include <unordered_map>

#include <musicplayer/chipplayer.h>

using namespace std;
using namespace utils;

namespace chipmachine {

MusicPlayerList::MusicPlayerList(const std::string &workDir) : mp(workDir) {
	SET_STATE(STOPPED);
	wasAllowed = true;
	permissions = 0xff;
	quitThread = false;
	playerThread = thread([=] {
		while(!quitThread) {
			plMutex.lock();
			if(funcs.size()) {
				auto q = funcs;
				funcs.clear();
				plMutex.unlock();
				for(auto &f : q)
					f();
			} else
				plMutex.unlock();
			update();
			sleepms(50);
		}
	});
}

void MusicPlayerList::addSong(const SongInfo &si, bool shuffle) {

	//LOCK_GUARD(plMutex);
	onThisThread([=] {
		if(shuffle) {
			playList.insertAt(rand() % (playList.size() + 1), si);
		} else {
			LOGD("PUSH %s/%s (%s)", si.title, si.composer, si.path);
			playList.push_back(si);
		}
	});
	//return true;
}

void MusicPlayerList::clearSongs() {
	//LOCK_GUARD(plMutex);
	onThisThread( [=] { playList.clear(); });
}

void MusicPlayerList::nextSong() {
	//LOCK_GUARD(plMutex);
	onThisThread([=] {
		if(playList.size() > 0) {
			// mp.stop();
			SET_STATE(WAITING);
		}
	});
}

void MusicPlayerList::playSong(const SongInfo &si) {
	onThisThread([=] {
		dbInfo = currentInfo = si;
		SET_STATE(PLAY_NOW);
	});
}

void MusicPlayerList::seek(int song, int seconds) {
	onThisThread([=] {
		if(multiSongs.size()) {
			LOGD("CHANGED MULTI");
			state = PLAY_MULTI;
			multiSongNo = song;
			return;
		}
		mp.seek(song, seconds);
		if(song >= 0)
			changedSong = true;
	});
}

uint16_t *MusicPlayerList::getSpectrum() {
	return mp.getSpectrum();
}

int MusicPlayerList::spectrumSize() {
	return mp.spectrumSize();
}

SongInfo MusicPlayerList::getInfo(int index) {
	LOCK_GUARD(plMutex);
	if(index == 0)
		return currentInfo;
	return playList.getSong(index - 1);
}

SongInfo MusicPlayerList::getDBInfo() {
	LOCK_GUARD(plMutex);
	return dbInfo;
}

int MusicPlayerList::getLength() {
	return playerLength;
}

int MusicPlayerList::getPosition() {
	return playerPosition;
}


int MusicPlayerList::listSize() {
	LOCK_GUARD(plMutex);
	return playList.size();
}

/// PRIVATE

void MusicPlayerList::updateInfo() {
	auto si = mp.getPlayingInfo();
	if(si.format != "")
		currentInfo.format = si.format;
	if(!multiSongs.size()) {
		currentInfo.numtunes = si.numtunes;
		currentInfo.starttune = si.starttune;
	}
}

bool MusicPlayerList::handlePlaylist(const string &fileName) {

	playList.clear();
	File f{fileName};

	auto lines = f.getLines();

	// Remove lines with comment character
	lines.erase(std::remove_if(lines.begin(), lines.end(), [=](const string &l) {
		            return l[0] == ';';
		        }), lines.end());
	for(const string &s : lines) {
		playList.push_back(SongInfo(s));
	}

    if(playList.size() == 0)
        return false;

	MusicDatabase::getInstance().lookup(playList.front());
	if(playList.front().path == "") {
		LOGD("Could not lookup '%s'", playList.front().path);
		errors.push_back("Bad song in playlist");
		SET_STATE(ERROR);
		return false;
	}
	SET_STATE(WAITING);
	return true;
}

bool MusicPlayerList::playFile(const std::string &fn) {
	auto fileName = fn;
	if(fileName == "")
		return false;
	auto ext = toLower(path_extension(fileName));
	if(ext == "pls" || currentInfo.format == "PLS") {
		File f{fileName};

		auto lines = f.getLines();
		vector<string> result;
		for(auto &l : lines) {
			if(startsWith(l, "File1="))
				result.push_back(l.substr(6));
		}
		currentInfo.path = result[0];
		currentInfo.format = "MP3";
		playCurrent();
		return false;

	} else if(ext == "m3u" || currentInfo.format == "M3U") {
		File f{fileName};

		auto lines = f.getLines();

		// Remove lines with comment character
		lines.erase(std::remove_if(lines.begin(), lines.end(), [=](const string &l) {
			            return l == "" || l[0] == '#';
			        }), lines.end());
		currentInfo.path = lines[0];
		currentInfo.format = "MP3";
		playCurrent();
		return false;

	} else if(ext == "plist") {
		handlePlaylist(fileName);
		return true;
	} else if(ext == "jb") {
		// Jason Brooke fix
		string newName = fileName.substr(0, fileName.find_last_of('.')) + ".jcb";
		if(!File::exists(newName))
			File::copy(fileName, newName);
		fileName = newName;
	}

	if(mp.playFile(fileName)) {
#ifdef USE_REMOTELISTS
		if(reportSongs)
			RemoteLists::getInstance().songPlayed(currentInfo.path);
#endif
		if(currentInfo.starttune >= 0)
			mp.seek(currentInfo.starttune);
		changedSong = false;
		LOGD("CHANGED MULTI:%s", changedMulti ? "YES" : "NO");
		if(!changedMulti) {
			updateInfo();
			LOGD("STATE: Play started");
			SET_STATE(PLAY_STARTED);
		} else
			SET_STATE(PLAYING);

		bitRate = 0;

		changedMulti = false;
		return true;
	} else {
		errors.push_back("Could not play song");
		SET_STATE(ERROR);
	}
	return false;
}

void MusicPlayerList::setPartyMode(bool on, int lockSec, int graceSec) {
	partyMode = on;
	partyLockDown = false;
	setPermissions(0xffff);
	lockSeconds = lockSec;
	graceSeconds = graceSec;
}

void MusicPlayerList::cancelStreaming() {
	RemoteLoader::getInstance().cancel();
	mp.clearStreamFifo();
}
void MusicPlayerList::update() {

	LOCK_GUARD(plMutex);

	mp.update();

	RemoteLoader::getInstance().update();

	if(partyMode) {
		auto p = getPosition();
		if(partyLockDown) {
			if(p >= lockSeconds) {
				setPermissions(0xffff);
				partyLockDown = false;
			}
		} else {
			if(p >= graceSeconds && p < lockSeconds) {
				partyLockDown = true;
				setPermissions(CAN_PAUSE | CAN_ADD_SONG | PARTYMODE);
			}
		}
	}

	if(state == PLAY_NOW) {
		SET_STATE(STARTED);
		// LOGD("##### PLAY NOW: %s (%s)", currentInfo.path, currentInfo.title);
		multiSongs.clear();
		playedNext = false;
		playCurrent();
	}

	if(state == PLAY_MULTI) {
		SET_STATE(STARTED);
		currentInfo.path = multiSongs[multiSongNo];
		changedMulti = true;
		playCurrent();
	}

	if(state == PLAYING || state == PLAY_STARTED) {

		auto pos = mp.getPosition();
		auto length = mp.getLength();

		if(cueSheet) {
			subtitle = cueSheet->getTitle(pos);
			subtitlePtr = subtitle.c_str();
		}

		if(!changedSong && playList.size() > 0) {
			if(!mp.playing()) {
				if(playList.size() == 0)
					SET_STATE(STOPPED);
				else
					SET_STATE(WAITING);
			} else if((length > 0 && pos > length) && pos > 7) {
				LOGD("STATE: Song length exceeded");
				mp.fadeOut(3.0);
				SET_STATE(FADING);
			} else if(detectSilence && mp.getSilence() > 44100 * 6 && pos > 7) {
				LOGD("STATE: Silence detected");
				mp.fadeOut(0.5);
				SET_STATE(FADING);
			}
		} else if(partyLockDown) {
			if((length > 0 && pos > length) || mp.getSilence() > 44100 * 6) {
				partyLockDown = false;
				setPermissions(0xffff);
			}
		}
	}

	if(state == FADING) {
		if(mp.getFadeVolume() <= 0.01) {
			LOGD("STATE: Music ended");
			if(playList.size() == 0)
				SET_STATE(STOPPED);
			else
				SET_STATE(WAITING);
		}
	}

	if(state == LOADING) {
		if(files == 0) {
			cancelStreaming();
			playFile(loadedFile);
		}
	}

	if(state == WAITING && (playList.size() > 0)) {
		SET_STATE(STARTED);
		playedNext = true;
		dbInfo = currentInfo = playList.front();
		playList.pop_front();

		if(playList.size() > 0) {
			// Update info for next song from
			MusicDatabase::getInstance().lookup(playList.front());
		}
			
		// pos = 0;
		LOGD("Next song from queue : %s (%d)", currentInfo.path, currentInfo.starttune);
		if(partyMode) {
			partyLockDown = true;
			setPermissions(CAN_PAUSE | CAN_ADD_SONG | PARTYMODE);
		}
		multiSongs.clear();
		playCurrent();
	}

	// Cache values for outside access

	playerPosition = mp.getPosition();
	playerLength = mp.getLength(); // currentInfo.length;

	if(multiSongs.size())
		currentTune = multiSongNo;
	else
		currentTune = mp.getTune();

	playing = mp.playing();
	paused = mp.isPaused();
	auto br = mp.getMeta("bitrate");
	if(br != "") {
		bitRate = std::stol(br);
	}

	if(!cueSheet) {
		subtitle = mp.getMeta("sub_title");
		subtitlePtr = subtitle.c_str();
	}
}

void MusicPlayerList::playCurrent() {

	SET_STATE(LOADING);
	
	songFiles.clear();
	//screenshot = "";
	
	LOGD("PLAY PATH:%s", currentInfo.path);
	string prefix, path;
	auto parts = split(currentInfo.path, "::", 2);
	if(parts.size() == 2) {
		prefix = parts[0];
		path = parts[1];
	} else
		path = currentInfo.path;
	
	
	if(prefix == "index") {
		int index = stol(path);
		dbInfo = currentInfo = MusicDatabase::getInstance().getSongInfo(index);
		auto parts = split(currentInfo.path, "::", 2);
		if(parts.size() == 2) {
			prefix = parts[0];
			path = parts[1];
		} else
			path = currentInfo.path;
	}

	if(prefix == "product") {
		auto id = stol(path);
		playList.psongs.clear();
		for(const auto &song : MusicDatabase::getInstance().getProductSongs(id)) {
			playList.psongs.push_back(song);
		}
    	if(playList.psongs.size() == 0) {
			LOGD("No songs in product");
			errors.push_back("No songs in product");
			SET_STATE(ERROR);
        	return;
		}

		// Check that the first song is working
		MusicDatabase::getInstance().lookup(playList.psongs.front());
		if(playList.psongs.front().path == "") {
			LOGD("Could not lookup '%s'",playList.psongs.front().path);
			errors.push_back("Bad song in product");
			SET_STATE(ERROR);
			return;
		}
		SET_STATE(WAITING);
		return;
	} else {
		if(currentInfo.metadata[SongInfo::SCREENSHOT] == "") {
			auto s = MusicDatabase::getInstance().getSongScreenshots(currentInfo);
			currentInfo.metadata[SongInfo::SCREENSHOT] = s;
		}
	}

	if(prefix == "playlist") {
		if(!handlePlaylist(path))
			SET_STATE(ERROR);
		return;
	}

	if(startsWith(path, "MULTI:")) {
		multiSongs = split(path.substr(6), "\t");
		if(prefix != "") {
			for(string &m : multiSongs) {
				m = prefix + "::" + m;
			}
		}
		multiSongNo = 0;
		currentInfo.path = multiSongs[0];
		currentInfo.numtunes = multiSongs.size();
		playCurrent();
		return;
	}


	auto ext = path_extension(path);
	makeLower(ext);

	detectSilence = true;
	if(ext == "mp3")
		detectSilence = false;

	cueSheet = nullptr;
	subtitle = "";
	subtitlePtr = subtitle.c_str();

	playerPosition = 0;
	playerLength = 0;
	bitRate = 0;
	currentTune = 0;

	cancelStreaming();

	if(File::exists(currentInfo.path)) {
		LOGD("PLAYING LOCAL FILE %s", currentInfo.path);
		songFiles = { File(currentInfo.path) };
		loadedFile = currentInfo.path;
		files = 0;
		return;
	}

	loadedFile = "";
	files = 0;
	RemoteLoader &loader = RemoteLoader::getInstance();

	string cueName = "";
	if(prefix == "bitjam")
		cueName = currentInfo.path.substr(0, currentInfo.path.find_last_of('.')) + ".cue";
	else if(prefix == "demovibes")
		cueName = toLower(currentInfo.path.substr(0, currentInfo.path.find_last_of('.')) + ".cue");

	if(cueName != "") {
		loader.load(cueName, [=](File cuefile) {
			if(cuefile)
				cueSheet = make_shared<CueSheet>(cuefile);
		});
	}
	
	if(startsWith(currentInfo.path, "pouet::")) {
		loadedFile = currentInfo.path.substr(7);
		files = 0;
		return;
	}

	if(currentInfo.format != "M3U" && (ext == "mp3" || toLower(currentInfo.format) == "mp3")) {

		if(mp.streamFile("dummy.mp3")) {
			SET_STATE(PLAY_STARTED);
			LOGD("Stream start");
			string name = currentInfo.path;
			loader.stream(currentInfo.path, [=](int what, const uint8_t *ptr, int n) -> bool {
				if(what == RemoteLoader::PARAMETER) {
					mp.setParameter((char *)ptr, n);
				} else {
					//LOGD("Writing to %s", name);
					mp.putStream(ptr, n);
				}
				return true;
			});
		}
		return;
	}

	bool isStarTrekker = (currentInfo.path.find("Startrekker") != string::npos);

	// Known music formats with 2 files
	static const std::unordered_map<string, string> fmt_2files = {
	    {"mdat", "smpl"},   // TFMX
	    {"sng", "ins"},     // Richard Joseph
	    {"jpn", "smp"},     // Jason Page PREFIX
	    {"dum", "ins"},     // Rob Hubbard 2
	    {"adsc", "adsc.as"}, // Audio Sculpture
	    {"sdata", "ip"} // Audio Sculpture
	};
	string ext2;
	if(fmt_2files.count(ext) > 0)
		ext2 = fmt_2files.at(ext);
	if(isStarTrekker)
		ext2 = "mod.nt";
	if(!ext2.empty()) {
		files++;
		auto smpl_file = currentInfo.path.substr(0, currentInfo.path.find_last_of('.') + 1) + ext2;
		LOGD("Loading secondary (sample) file '%s'", smpl_file);
		loader.load(smpl_file, [=](File f) {
			if(!f) {
				errors.emplace_back("Could not load secondary file");
				SET_STATE(ERROR);
			};
			songFiles.push_back(f);
			files--;
		});
	}

	// LOGD("LOADING:%s", currentInfo.path);
	files++;
	loader.load(currentInfo.path, [=](File f0) {
		if(!f0) {
			errors.emplace_back("Could not load file");
			SET_STATE(ERROR);
			files--;
			return;
		}
		songFiles.push_back(f0);
		loadedFile = f0.getName();
		auto ext = toLower(path_extension(loadedFile));
		LOGD("Loaded file '%s'", loadedFile);
		auto parentDir = File(path_directory(loadedFile));
		auto fileList = mp.getSecondaryFiles(f0);
		for(const auto &s : fileList) {
			File target = parentDir / s;
			if(!target.exists()) {
				files++;
				RemoteLoader &loader = RemoteLoader::getInstance();
				auto url = path_directory(currentInfo.path) + "/" + s;
				loader.load(url, [=](File f) {
					if(!f) {
						errors.emplace_back("Could not load file");
						SET_STATE(ERROR);
					} else {
						LOGD("Copying secondary file to %s", target.getName());
						File::copy(f.getName(), target);
						songFiles.push_back(target);
					}
					files--;
				});
			} else
				songFiles.push_back(target);
		}

		files--;
	});
}

} // namespace chipmachine
