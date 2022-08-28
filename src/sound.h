#ifndef _SOUND_H_
#define _SOUND_H_

#include <condition_variable>
#include <map>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vlc/vlc.h>

class Sound {
public:
  enum class Track { GunShoot, RifleShoot, Running, FallDown, GruntingHit };

  static Sound &instance();

  unsigned int play(Track track) const;
  void stop(Track track) const;

private:
  Sound();
  ~Sound();

  void add_track(Track track, const std::string &file,
                 unsigned int duration_ms);

private:
  struct TrackInfo {
    unsigned int play() const;
    void stop() const;

    libvlc_media_player_t *media_player;
    unsigned int duration_ms;
  };

  libvlc_instance_t *m_vlc_engine;
  std::unordered_map<Track, TrackInfo> m_media_players;
};

class SoundPlayer {
public:
  // SoundPlayer is a singleton
  static SoundPlayer &instance();

  void play_track(Sound::Track track);
  void stop_track(Sound::Track track);

private:
  // private constructor of a singleton
  SoundPlayer();
  ~SoundPlayer();

  void play_loop();

  void remove_finished_jobs(long time_now_ms,
                            const std::unordered_set<Sound::Track> &stop_jobs);

  void start_new_jobs(long time_now_ms,
                      const std::unordered_set<Sound::Track> &start_jobs);

private:
  // thread that is executing a play loop
  std::thread m_thread;

  // tracks to be played (needs to be thread safe)
  std::unordered_set<Sound::Track> m_start_jobs;
  // tracks to be stopped (needs to be thread safe)
  std::unordered_set<Sound::Track> m_stop_jobs;
  // tracks that are currently playing (accessed only by the thread that is
  // running play loop)
  std::unordered_map<Sound::Track, long> m_running_jobs;

  // tells a thread to stop looking for jobs (needs to be thread safe)
  bool m_should_terminate = false;

  // prevents data races to the m_start_jobs and m_stop_jobs queues and
  // m_should_terminate
  std::mutex m_queue_mutex;
  // condition that needs to be satisfied in order to thread to continue working
  std::condition_variable m_mutex_condition;
};

#endif /* _SOUND_H_ */
