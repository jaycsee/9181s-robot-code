#include "main.h"
#include <cstring>
#include <regex>
#include <stdexcept>
#include <utility>
#include "controller.hpp"

void RecordedController::task(void* param) {
  RecordedController * rc = static_cast<RecordedController*>(param);
  unsigned long prev_time = pros::millis();
  while (flags::generalAlive) {
    int millis = pros::millis();
    std::string curr_time = std::to_string(std::floor(millis / 60)) + ":" + std::to_string(std::floor(millis % 60));
    std::string time_diff = std::to_string(std::floor((millis - rc->textTime) / 60)) + ":" + std::to_string(std::floor((millis - rc->textTime) % 60));
    for (unsigned int i = 0; i < 3; i++) {
      rc->watcher->notify();
      rc->controller->set_text(i, 0, std::regex_replace(std::regex_replace(rc->textBuffer.at(i), std::regex("{time}"), curr_time), std::regex("{timediff}"), time_diff).c_str());
      pros::Task::delay_until(&prev_time, 1000 / TASK_CONTROLLER_TEXT_HZ);
    }
  }
}

int RecordedController::popFront(std::vector<int> & stack) {
  int get = stack.front();
  stack.erase(stack.begin());
  return get;
}

bool RecordedController::readFile(std::string file) {
  std::ifstream ifs;
  ifs.open(file);
  if (!ifs.is_open()) return false;

  std::vector<std::string> in;
  std::string temp;
  while (std::getline(ifs, temp))
    in.push_back(temp);

  try {
    if (std::stoi(in.back()) != TASK_OPCONTROL_HZ)
      Watchdog::alert(LOG_WARNING, "The recording is at a different frequency than the one configured. Playback timings may be incorrect", "PLAYBACK FREQ|" + RecordedController::name + ": " + in.back() + " >> " + std::to_string(TASK_OPCONTROL_HZ));
  } catch (std::invalid_argument & e) {}

  in.erase(in.end() - 1);

  std::string header = std::string(in.front());
  std::vector<int> keys;
  while (header.size() != 0) {
    std::pair<std::string, std::string> separated = util::separateFirst(header, ",");
    std::string add = separated.first;
    header = separated.second;
    try {
      keys.push_back(std::stoi(add));
    } catch (std::invalid_argument & e) {
      keys.push_back(-1);
      Watchdog::alert(LOG_WARNING, "The channel \"" + add + "\" could not be parsed for playback. Any associated values to this channel will be ignored", "PLAYBACK IHEAD|" + RecordedController::name + ": " + add);
    }
  }
  in.erase(in.begin());

  for (auto i : keys) {
    std::vector<std::string> updated;
    std::vector<int> stack;
    bool finished = false;
    for (auto s : in) {
      std::pair<std::string, std::string> separated = util::separateFirst(s, ",");
      std::string add = separated.first;
      try {
        stack.push_back(std::stoi(add));
      } catch (std::invalid_argument & e) {
        stack.push_back(0);
        Watchdog::alert(LOG_WARNING, "The value \"" + add + "\" for channel" + std::to_string(i) + "could not be parsed for playback. Requesting this value will return 0", "PLAYBACK IVALUE|" + RecordedController::name + "(" + std::to_string(i) + "): " + add);
      }

      if (separated.second.size() != 0)
        updated.push_back(separated.second);
      else
        finished = true;
    }

    if (i != -1)
      if (i < 5)
        analogStack.insert(std::pair<int, std::vector<int>>(i, stack));
      else
        digitalStack.insert(std::pair<int, std::vector<int>>(i, stack));

    in = updated;

    if (finished) break;
  }

  return true;
}

bool RecordedController::readStreams(std::unordered_map<int, std::string> streams) {
  std::unordered_map<int, std::vector<std::string>> in;
  for (auto pair : streams) {
    std::ifstream ifs;
    ifs.open(pair.second);
    if (!ifs.is_open()) {
      Watchdog::alert(LOG_WARNING, "The file \"" + pair.second + "\" could not be found for channel " + std::to_string(pair.first) + ". This channel will not be saved", "STREAM SAVE NF|" + pair.second);
      continue;
    }
    std::vector<std::string> lines;
    std::string temp;
    while (std::getline(ifs, temp))
      lines.push_back(temp);
    in.insert(std::pair<int, std::vector<std::string>>(pair.first, lines));
  }

  for (auto pair : in) {
    std::vector<int> values;
    for (auto s : pair.second) {
      std::string sFirst = util::separateFirst(s, ",").first;
      std::string sSecond = util::separateFirst(s, ",").second;
      while (sFirst.size() != 0) {
        try {
          values.push_back(std::stoi(sFirst));
        } catch (std::exception & e) {
          values.push_back(0);
          Watchdog::alert(LOG_WARNING, "The value \"" + sFirst + "\" for channel" + std::to_string(pair.first) + "could not be parsed. 0 will be saved instead.", "STREAM IVALUE|" + sFirst);
        }
        std::pair<std::string, std::string> newPair = util::separateFirst(sSecond, ",");
        sFirst = newPair.first;
        sSecond = newPair.second;
      }
    }
    if (pair.first < 5)
      analogStack.insert(std::pair<int, std::vector<int>>(pair.first, values));
    else
      digitalStack.insert(std::pair<int, std::vector<int>>(pair.first, values));
  }
  return true;
}

void RecordedController::writeFile(std::string file) {
  int maxsize = 0;

  for (auto pair : RecordedController::analogStack)
    maxsize = util::max(maxsize, pair.second.size());
  for (auto pair : RecordedController::digitalStack)
    maxsize = util::max(maxsize, pair.second.size());

  for (auto pair : RecordedController::analogStack)
    while (pair.second.size() < maxsize)
      pair.second.push_back(0);
  for (auto pair : RecordedController::digitalStack)
    while (pair.second.size() < maxsize)
      pair.second.push_back(0);

  std::vector<std::string> out (maxsize + 1);

  int index;
  for (auto pair : RecordedController::analogStack) {
    out.at(index).append(std::to_string(pair.first) + ",");
    index++;
    for (auto i : pair.second) {
      out.at(index).append(std::to_string(i) + ",");
    }
    index = 0;
  }

  for (auto pair : RecordedController::digitalStack) {
    out.at(index).append(std::to_string(pair.first) + ",");
    index++;
    for (auto i : pair.second) {
      out.at(index).append(std::to_string(i) + ",");
    }
    index = 0;
  }

  out.push_back(std::to_string(TASK_OPCONTROL_HZ));

  std::ofstream of;
  of.open(RecordedController::recfile);

  for (auto s : out)
    of << s << "\n";

  of.close();
}

RecordedController::RecordedController(std::string name, pros::controller_id_e_t id) {
  pros::Controller * c = new pros::Controller(id);
  RecordedController::name = name;
  RecordedController::controller = c;
}

int RecordedController::clear() {
  return controller->clear();
}

int RecordedController::clear_line(int line) {
  return controller->clear_line(line);
}

int RecordedController::is_connected() {
  return controller->is_connected() || state == 2;
}

int RecordedController::get_analog(pros::controller_analog_e_t channel) {
  if (state == 2) {
    if (RecordedController::analogStack.find(channel) != RecordedController::analogStack.end()) {
      std::vector<int> stack = analogStack.at(channel);
      int get = RecordedController::popFront(stack);
      analogStack[channel] = stack;
      if (stack.size() == 0) {
        RecordedController::playbackComplete();
        Watchdog::alert(LOG_INFO, "Playback complete", "PLAYBACK COMP|" + RecordedController::name);
        RecordedController::state = 0;
      }
      return get;
    } else {
      Watchdog::alert(LOG_ERROR, "Requested value for undefined channel \"" + std::to_string(channel) + "\"", "PLAYBACK ICHA|" + RecordedController::name + ": " + std::to_string(channel));
      RecordedController::playback();
    }
  }
  int get = RecordedController::controller->get_analog(channel);
  if (state == 1) {
    if (RecordedController::analogStack.find(channel) == RecordedController::analogStack.end())
      RecordedController::analogStack.insert(std::pair<int, std::vector<int>>(channel, std::vector<int>()));
    RecordedController::analogStack.at(channel).push_back(get);
  }
  if (state == 3) {
    if (RecordedController::streamOutput.find(channel) == RecordedController::streamOutput.end()) {
      std::string copy = streamfile;
      copy = std::regex_replace(copy, std::regex("{c}"), std::to_string(channel));
      RecordedController::streamFiles.insert(std::pair<int, std::string>(channel, copy));
      RecordedController::streamOutput.insert(std::pair<int, std::ofstream>(channel, std::ofstream(copy)));
    }
    RecordedController::streamOutput.at(channel) << std::to_string(get) << ",";
  }
  return get;
}

int RecordedController::get_battery_capacity() {
  return controller->get_battery_capacity();
}

int RecordedController::get_battery_level() {
  return controller->get_battery_level();
}

int RecordedController::get_digital(pros::controller_digital_e_t button) {
  if (state == 2) {
    if (RecordedController::digitalStack.find(button) != RecordedController::digitalStack.end()) {
      std::vector<int> stack = digitalStack.at(button);
      int get = RecordedController::popFront(stack);
      digitalStack[button] = stack;
      if (stack.size() == 0) {
        RecordedController::playbackComplete();
        Watchdog::alert(LOG_INFO, "Playback complete", "PLAYBACK COMP|" + RecordedController::name);
        RecordedController::state = 0;
      }
      RecordedController::digitalLastKnown[button] = get;
      return get;
    } else {
      Watchdog::alert(LOG_ERROR, "Requested value for undefined button \"" + std::to_string(button) + "\"");
      RecordedController::playback();
    }
  }
  int get = RecordedController::controller->get_digital(button);
  RecordedController::digitalLastKnown[button] = get;
  if (state == 1) {
    if (RecordedController::digitalStack.find(button) == RecordedController::digitalStack.end())
      RecordedController::digitalStack.insert(std::pair<int, std::vector<int>>(button, std::vector<int>()));
    RecordedController::digitalStack.at(button).push_back(get);
  }
  if (state == 3) {
    if (RecordedController::streamOutput.find(button) == RecordedController::streamOutput.end()) {
      std::string copy = streamfile;
      copy = std::regex_replace(copy, std::regex("{c}"), std::to_string(button));
      RecordedController::streamFiles.insert(std::pair<int, std::string>(button, copy));
      RecordedController::streamOutput.insert(std::pair<int, std::ofstream>(button, std::ofstream(copy)));
    }
    RecordedController::streamOutput.at(button) << std::to_string(get) << ",";
  }
  return get;
}

int RecordedController::get_digital_new_press(pros::controller_digital_e_t button) {
  int lk;
  if (RecordedController::digitalLastKnown.find(button) != RecordedController::digitalLastKnown.end())
    lk = RecordedController::digitalLastKnown[button];
  else lk = -1;
  return lk == RecordedController::get_digital(button);
}

int RecordedController::set_text(std::uint8_t line, std::string str) {
  RecordedController::textTime = pros::millis();
  while (RecordedController::textBuffer.size() < 3)
    RecordedController::textBuffer.push_back("");
  line = util::min(line, 2);
  RecordedController::textBuffer.assign(line, str);
}

int RecordedController::set_text(std::vector<std::string> text) {
  RecordedController::textTime = pros::millis();
  RecordedController::textBuffer = text;
  return 1;
}

void RecordedController::record(bool record) {
  if (!SD_INSERTED)
    return;
  if (state == 0 && record) {
    RecordedController::analogStack.clear();
    RecordedController::digitalStack.clear();
    recfile = "/usd/recordings/" + util::timestamp() + ".csv";
    Watchdog::alert(LOG_INFO, "Now recording to: " + recfile, "RECORD START|" + RecordedController::name);
    RecordedController::state = 1;
  } else if (state == 1 && !record) {
    RecordedController::state = 0;
    Watchdog::alert(LOG_INFO, "Recording stopped, saving...", "RECORD STOP|" + RecordedController::name);
    RecordedController::writeFile(recfile);
    Watchdog::alert(LOG_INFO, "Recording saved", "RECORD SAVED|" + RecordedController::name);
  }
}

void RecordedController::stream(bool stream) {
  if (!SD_INSERTED)
    return;
  if (state == 0 && stream) {
    RecordedController::streamOutput.clear();
    recfile = "/usd/recordings/" + util::timestamp() + "-stream.csv";
    streamfile = "/usd/recordings/" + util::timestamp() + "-c{c}-unsaved.cstream";
    Watchdog::alert(LOG_INFO, "Now streaming", "STREAM START|" + RecordedController::name);
    RecordedController::state = 3;
  } else if (state == 3 && !stream) {
    RecordedController::state = 0;
    Watchdog::alert(LOG_INFO, "Streaming stopped, saving...", "STREAM STOP|" + RecordedController::name);
    RecordedController::streamOutput.clear();
    RecordedController::readStreams(streamFiles);
    RecordedController::writeFile(recfile);
    Watchdog::alert(LOG_INFO, "Stream saved", "STREAM SAVED|" + RecordedController::name);
  }
}

void RecordedController::playback(std::string file, void (*f)()) {
  if (state == 1)
    return;
  if (!RecordedController::readFile(file)) {
    Watchdog::alert(LOG_ERROR, "Recording file at \"" + file + "\" could not be found. Playback aborted", "PLAYBACK NF|" + file);
    return;
  }
  Watchdog::alert(LOG_INFO, "Playback started", "PLAYBACK START|" + RecordedController::name);

  RecordedController::playbackComplete = f;
  RecordedController::state = 2;
}

void RecordedController::playback() {
  if (state != 2)
    return;
  RecordedController::analogStack.clear();
  RecordedController::digitalStack.clear();
  Watchdog::alert(LOG_WARNING, "Playback aborted", "PLAYBACK ABORT|" + RecordedController::name);
  RecordedController::state = 0;
}