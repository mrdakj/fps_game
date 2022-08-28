#ifndef _TIMER_H_
#define _TIMER_H_

class Timer {
public:
  Timer();
  float tick(float current_time);

  void reset();

private:
  float m_last_update_time;
};

#endif /* _TIMER_H_ */
