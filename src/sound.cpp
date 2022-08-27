#include "sound.h"
#include <cassert>
#include <chrono>
#include <thread>
#include <vector>

#define GUN_FILE ("../res/sound/gun.mp3")
#define RIFLE_FILE ("../res/sound/rifle.mp3")
#define RUNNING_FILE ("../res/sound/running.mp3")

#define GUN_DURATION_MS (300)
#define RIFLE_DURATION_MS (300)
#define RUNNING_DURATION_MS (16000)

inline long get_time_now_ms() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

// ---------------- Sound ----------------------------
unsigned int Sound::TrackInfo::play() const {
  assert(media_player && "media player valid");
  libvlc_media_player_play(media_player);
  return duration_ms;
}

void Sound::TrackInfo::stop() const {
  assert(media_player && "media player valid");
  libvlc_media_player_stop(media_player);
}

Sound &Sound::instance() {
  static Sound instance;
  return instance;
}

unsigned int Sound::play(Track track) const {
  auto media_player_it = m_media_players.find(track);
  assert(media_player_it != m_media_players.end() &&
         media_player_it->second.media_player && "sound track found");
  return media_player_it->second.play();
}

void Sound::stop(Track track) const {
  auto media_player_it = m_media_players.find(track);
  assert(media_player_it != m_media_players.end() &&
         media_player_it->second.media_player && "sound track found");
  media_player_it->second.stop();
}

Sound::Sound() {
  // load the vlc endgine
  m_vlc_engine = libvlc_new(0, NULL);

  add_track(Track::GunShoot, GUN_FILE, GUN_DURATION_MS);
  add_track(Track::RifleShoot, RIFLE_FILE, RIFLE_DURATION_MS);
  add_track(Track::Running, RUNNING_FILE, RUNNING_DURATION_MS);
}

void Sound::add_track(Track track, const std::string &file,
                      unsigned int duration_ms) {
  // create a new item
  libvlc_media_t *m = libvlc_media_new_path(m_vlc_engine, file.c_str());
  // create a media player playing environement
  m_media_players[track] = {libvlc_media_player_new_from_media(m), duration_ms};
  // no need to keep the media now
  libvlc_media_release(m);
}

Sound::~Sound() {
  // free media players
  for (auto &media_player : m_media_players) {
    media_player.second.stop();
    libvlc_media_player_release(media_player.second.media_player);
  }

  // free the engine
  libvlc_release(m_vlc_engine);
}

// ---------------- SoundPlayer ----------------------------
SoundPlayer &SoundPlayer::instance() {
  // initiated only once
  static SoundPlayer instance;
  return instance;
}

void SoundPlayer::play_track(Sound::Track track) {
  {
    // add a job under the lock because m_start_jobs is used in play loop that
    // is executing in a separate thread
    std::unique_lock<std::mutex> lock(m_queue_mutex);
    m_start_jobs.insert(track);
  }
  // notify a thread that is executing a play loop that there is some change
  m_mutex_condition.notify_one();
}

void SoundPlayer::stop_track(Sound::Track track) {
  {
    // add a job under the lock because m_stop_jobs is used in play loop that
    // is executing in a separate thread
    std::unique_lock<std::mutex> lock(m_queue_mutex);
    m_stop_jobs.insert(track);
  }
  // notify a thread that is executing a play loop that there is some change
  m_mutex_condition.notify_one();
}

// start play loop in a separate thread
SoundPlayer::SoundPlayer()
    : m_thread(std::thread(&SoundPlayer::play_loop, this)) {}

SoundPlayer::~SoundPlayer() {
  {
    // set m_should_terminate under the lock because it is used in play loop
    // that is executing in a separate thread
    std::unique_lock<std::mutex> lock(m_queue_mutex);
    m_should_terminate = true;
  }
  // notify a thread that is executing a play loop that there is some change
  m_mutex_condition.notify_all();
  // wait for thread to finish
  m_thread.join();
}

void SoundPlayer::play_loop() {
  std::unordered_set<Sound::Track> start_jobs;
  std::unordered_set<Sound::Track> stop_jobs;

  while (true) {
    {
      std::unique_lock<std::mutex> lock(m_queue_mutex);

      m_mutex_condition.wait(lock, [this] {
        return !m_start_jobs.empty() || !m_stop_jobs.empty() ||
               m_should_terminate;
      });

      if (m_should_terminate) {
        return;
      }

      // quickly take data in order to release a lock as soon as possible
      std::swap(m_start_jobs, start_jobs);
      std::swap(m_stop_jobs, stop_jobs);
    }

    auto time_now_ms = get_time_now_ms();
    remove_finished_jobs(time_now_ms, stop_jobs);
    start_new_jobs(time_now_ms, start_jobs);

    start_jobs.clear();
    stop_jobs.clear();
  }
}

void SoundPlayer::remove_finished_jobs(
    long time_now_ms, const std::unordered_set<Sound::Track> &stop_jobs) {
  // m_running_jobs is accessed only in this thread so there is no need for a
  // lock
  std::vector<Sound::Track> running_jobs_to_remove;
  for (const auto &running_job : m_running_jobs) {
    if (time_now_ms >= running_job.second ||
        stop_jobs.find(running_job.first) != stop_jobs.end()) {
      running_jobs_to_remove.push_back(running_job.first);
    }
  }

  for (Sound::Track track : running_jobs_to_remove) {
    Sound::instance().stop(track);
    m_running_jobs.erase(track);
  }
}

void SoundPlayer::start_new_jobs(
    long time_now_ms, const std::unordered_set<Sound::Track> &start_jobs) {
  // m_running_jobs is accessed only in this thread so there is no need for a
  // lock
  for (const auto &new_job : start_jobs) {
    if (m_running_jobs.find(new_job) == m_running_jobs.end()) {
      auto duration_ms = Sound::instance().play(new_job);
      m_running_jobs.emplace(new_job, time_now_ms + duration_ms);
    }
  }
}
