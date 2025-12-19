// meeting_scheduling.cpp : Defines the entry point for the application.
//

#include "meeting_scheduling.h"
#include <vector>
#include <string>
#include <iostream>
#include <cstdio>
#include <regex>
#include <fstream>
#include <random>
#include <ctime>
#include <algorithm>
#include <chrono>

using namespace std;

static int auto_id = 1;

static int hour_to_min(double hours) {
	int full_hours = (int) hours;
	double minute_segs = ((hours - full_hours) * 60) / 15;
	int full_minute_segs = (int) minute_segs;
	if (minute_segs - full_minute_segs != 0) ++full_minute_segs;
	return ((full_hours * 4) + full_minute_segs);
}

static int hour_to_min(string time) {
	int res = 0;
	std::regex time_format_12h("^ *(1[0-2]|0?\\d):([0-5]\\d) *(am|pm|AM|PM|Am|Pm) *$");
	std::regex time_format_24h("^ *(2[0-3]|[0-1]?\\d):([0-5]\\d) *$");
	std::regex time_format_h("^ *((?:2[0-3]|[0-1]?\\d)(?:\\.\\d*)?) *$");
	std::string str_hour = "";
	std::string str_min = "";
	std::string day_part = "n";
	if (std::regex_match(time, time_format_12h)) {
		str_hour = std::regex_replace(time, time_format_12h, "$1");
		str_min = std::regex_replace(time, time_format_12h, "$2");
		day_part = std::regex_replace(time, time_format_12h, "$3");
		if (str_hour == "12") day_part = "n";
	}
	else if (std::regex_match(time, time_format_24h)) {
		str_hour = std::regex_replace(time, time_format_24h, "$1");
		str_min = std::regex_replace(time, time_format_24h, "$2");
	}
	else if (std::regex_match(time, time_format_h)) {
		str_hour = std::regex_replace(time, time_format_h, "$1");
		return hour_to_min(std::stod(str_hour));
	}
	else return -1;

	res = std::stoi(str_min);
	res += (int) (res / 15);
	res += (res % 15 != 0);
	res = std::stoi(str_hour) * 4;
	res += (day_part.at(0) == 'p' || day_part.at(0) == 'P') * 48;
	return res;
};

class meeting {
public:
	int id_;
	// 24-h clock with 15 minute specificity
	int start_time_;
	// hours with 15 minute specificity
	int duration_;

	meeting(double start_time, double duration) : start_time_(hour_to_min(start_time)), duration_(hour_to_min(duration)) {
		if (start_time < 0.0 || start_time > 24.0 || start_time + duration > 24.0 || duration <= 0) throw std::exception("Invalid time"); 
		id_ = auto_id; 
		++auto_id;
	};

	meeting(string start_time, string duration) : start_time_(hour_to_min(start_time)), duration_(hour_to_min(duration)) {
		if (start_time_ < 0 || start_time_ > 96 || start_time_ + duration_ > 96 || duration_ <= 0) throw std::exception("Invalid time");
		id_ = auto_id;
		++auto_id;
	};

	// Prioritizing earliest starting time followed by shortest duration
	bool less_etsd(meeting& rhs) const {
		if (this->start_time_ != rhs.start_time_) return this->start_time_ < rhs.start_time_;
		return this->duration_ < rhs.duration_;
	};

	// Prioritizing shortest duration followed by earliest time
	bool less_sdet(meeting& rhs) const {
		if (this->duration_ != rhs.duration_) return this->duration_ < rhs.duration_;
		return this->start_time_ < rhs.start_time_;
	}
	
	// Prioritizing latest end, shortest duration
	bool less_lesd(meeting& rhs) const {
		if (this->start_time_ + this->duration_ != rhs.start_time_ + rhs.duration_) return this->start_time_ + this->duration_ > rhs.start_time_ + rhs.duration_;
		return this->duration_ < rhs.duration_;
	}

	// Prioritizing latest end, longest duration
	bool less_leld(meeting& rhs) const {
		if (this->start_time_ + this->duration_ != rhs.start_time_ + rhs.duration_) return this->start_time_ + this->duration_ > rhs.start_time_ + rhs.duration_;
		return this->duration_ > rhs.duration_;
	}

	// Prioritizing shortest duration, latest end
	bool less_sdle(meeting& rhs) const {
		if (this->duration_ != rhs.duration_) return this->duration_ < rhs.duration_;
		return this->start_time_ + this->duration_ > rhs.start_time_ + rhs.duration_;
	}

	//Prioritizing longest duration, latest end
	bool less_ldle(meeting& rhs) const {
		if (this->duration_ != rhs.duration_) return this->duration_ > rhs.duration_;
		return this->start_time_ + this->duration_ > rhs.start_time_ + rhs.duration_;
	}

	std::string to_string() const {
		return "st: " + std::to_string(start_time_) + ", dur: " + std::to_string(duration_) + ", end: " + std::to_string(start_time_ + duration_);
	}
};

class meeting_room {
public:
	int id_;
	int start_time_;
	int duration_;
	std::vector<int> timeslots_;

	meeting_room(double start_time, double duration) : start_time_(hour_to_min(start_time)), duration_(hour_to_min(duration)) { 
		if (start_time < 0.0 || start_time > 24.0 || start_time + duration > 24.0) throw std::exception("Invalid time"); 
		id_ = auto_id;
		++auto_id;
		for (int i = 0; i < 96; ++i) {
			if (i < start_time_ || i > start_time_ + duration_) timeslots_.push_back(-1);
			else timeslots_.push_back(0);
		}
	};

	bool add_meeting(const meeting& mtg) {
		for (int i = mtg.start_time_; i < mtg.start_time_ + mtg.duration_; ++i) {
			if (timeslots_[i] != 0) return false;
		}

		for (int i = mtg.start_time_; i < mtg.start_time_ + mtg.duration_; ++i) {
			timeslots_[i] = mtg.id_;
		}

		return true;
	}

	int meetings_scheduled() const {
		int count = 0;
		int trailing = 0;
		for (int i = 0; i < timeslots_.size(); ++i) {
			if (timeslots_[i] != -1 && timeslots_[i] != 0 && timeslots_[i] != trailing) {
				++count;
				trailing = timeslots_[i];
			}
		}
		return count;
	}

	void reset() {
		for (int i = 0; i < 96; ++i) {
			if (i < start_time_ || i > start_time_ + duration_) timeslots_[i] = -1;
			else timeslots_[i] = 0;
		}
	}

	void print() const {
		int trailing = 0;
		std::cout << "Room " + std::to_string(this->id_) + " Schedule --------\nHour | Meeting ids\n-----|------------\n";
		for (int i = 0; i < timeslots_.size(); ++i) {
			if (timeslots_[i] != -1) {
				if (i % 4 == 0) {
					std::cout << std::to_string(i / 4);
					if (i < 40) std::cout << " ";
					std::cout << "   |--- ";
				}
				else {
					std::cout << "     |  - ";
				}
				if (timeslots_[i] == 0) std::cout << "\n";
				else {
					if (trailing == timeslots_[i]) std::cout << "[]\n";
					else std::cout << "Meeting " << std::to_string(timeslots_[i]) << "\n";
					trailing = timeslots_[i];
				}
			}
		}
	}
};

// Prioritizing earliest starting time followed by shortest duration
static void sort_meetings_etsd(std::vector<meeting>& meetings) {
	bool no_swap = false;
	for (int i = 0; i < meetings.size() - 1 && !no_swap; ++i) {
		no_swap = true;
		for (int j = i + 1; j < meetings.size(); ++j) {
			if (meetings[j].less_etsd(meetings[i])) {
				meeting temp = meetings[i];
				meetings[i] = meetings[j];
				meetings[j] = temp;
				no_swap = false;
			}
		}

	}
}

// Prioritizing shortest duration followed by earliest time
static void sort_meetings_sdet(std::vector<meeting>& meetings) {
	bool no_swap = false;
	for (int i = 0; i < meetings.size() - 1 && !no_swap; ++i) {
		no_swap = true;
		for (int j = i + 1; j < meetings.size(); ++j) {
			if (meetings[j].less_sdet(meetings[i])) {
				meeting temp = meetings[i];
				meetings[i] = meetings[j];
				meetings[j] = temp;
				no_swap = false;
			}
		}
	}
}

// Prioritizing latest end, shortest duration
static void sort_meetings_lesd(std::vector<meeting>& meetings) {
	bool no_swap = false;
	for (int i = 0; i < meetings.size() - 1 && !no_swap; ++i) {
		no_swap = true;
		for (int j = i + 1; j < meetings.size(); ++j) {
			if (meetings[j].less_lesd(meetings[i])) {
				meeting temp = meetings[i];
				meetings[i] = meetings[j];
				meetings[j] = temp;
				no_swap = false;
			}
		}
	}
}

// Prioritizing latest end, longest duration
static void sort_meetings_leld(std::vector<meeting>& meetings) {
	bool no_swap = false;
	for (int i = 0; i < meetings.size() - 1 && !no_swap; ++i) {
		no_swap = true;
		for (int j = i + 1; j < meetings.size(); ++j) {
			if (meetings[j].less_leld(meetings[i])) {
				meeting temp = meetings[i];
				meetings[i] = meetings[j];
				meetings[j] = temp;
				no_swap = false;
			}
		}
	}
}

// Prioritizing shortest duration, latest end
static void sort_meetings_sdle(std::vector<meeting>& meetings) {
	bool no_swap = false;
	for (int i = 0; i < meetings.size() - 1 && !no_swap; ++i) {
		no_swap = true;
		for (int j = i + 1; j < meetings.size(); ++j) {
			if (meetings[j].less_sdle(meetings[i])) {
				meeting temp = meetings[i];
				meetings[i] = meetings[j];
				meetings[j] = temp;
				no_swap = false;
			}
		}
	}
}

//Prioritizing longest duration, latest end
static void sort_meetings_ldle(std::vector<meeting>& meetings) {
	bool no_swap = false;
	for (int i = 0; i < meetings.size() - 1 && !no_swap; ++i) {
		no_swap = true;
		for (int j = i + 1; j < meetings.size(); ++j) {
			if (meetings[j].less_ldle(meetings[i])) {
				meeting temp = meetings[i];
				meetings[i] = meetings[j];
				meetings[j] = temp;
				no_swap = false;
			}
		}
	}
}

int main() {
	std::time_t now;
	std::time(&now);
	std::srand((unsigned int) now);
	meeting_room room(9, 9);
	std::vector<meeting> meetings;

	/*for (int i = 0; i < 50; ++i) {
		double start = (rand() % 9) + 9 + ((rand() % 4) / 4.0);
		double dur = (rand() % 9) + ((rand() % 5) / 4.0);
		if (start + dur > 18) dur = 18 - start;
		if (dur == 0) {
			dur = 1;
			--start;
		}
		meetings.push_back(meeting(start, dur));
	}*/

	//for (int i = 0; i < 100; ++i) {
	//	double start = (rand() % 24) + ((rand() % 4) / 4.0);
	//	while (start >= 23.75) {
	//		start = (rand() % 24) + ((rand() % 4) / 4.0);
	//	}
	//	double dur = 0;
	//	while (true) {
	//		dur = (rand() % 23) + ((rand() % 4) / 4.0);
	//		if (start + dur < 24 && dur > 0) break;
	//	}
	//	meetings.push_back(meeting(start, dur));
	//}

	//sort_meetings_etsd(meetings);
	//for (meeting target : meetings) {
	//	std::cout << target.to_string() << "\n";
	//}
	//std::cout << "\n\n";

	//return 0;
	//sort_meetings_sdet(meetings);
	//for (auto target : meetings) {
	//	std::cout << target.to_string() << "\n";
	//}
	//std::cout << "\n\n";

	//sort_meetings_lesd(meetings);
	//for (auto target : meetings) {
	//	std::cout << target.to_string() << "\n";
	//}
	//std::cout << "\n\n";

	//sort_meetings_leld(meetings);
	//for (auto target : meetings) {
	//	std::cout << target.to_string() << "\n";
	//}
	//std::cout << "\n\n";

	//return 0;

	double reps = 50000.0;

	int etsd = 0;
	int sdet = 0;
	int lesd = 0;
	int leld = 0;
	int sdle = 0;
	int ldle = 0;

	int start_before = 0;
	int start_after = 0;

	double total_duration = 0;

	auto process_start = std::chrono::high_resolution_clock::now();

	for (int i = 0; i < reps; ++i) {
		meetings.clear();

		for (int i = 0; i < 20; ++i) {
			double start = (rand() % 24) + ((rand() % 4) / 4.0);
			while (start >= 23.75) {
				start = (rand() % 24) + ((rand() % 4) / 4.0);
			}
			double dur = 0;
			while (start + dur >= 24 || dur <= 0) {
				dur = (rand() % 23) + ((rand() % 4) / 4.0);
			}
			start_before += (start < 12);
			start_after += (start >= 12);
			total_duration += dur;
			meetings.push_back(meeting(start, dur));
		}

		// Earliest time, shortest duration
		//std::cout << "Earliest time, shortest duration\n";

		sort_meetings_etsd(meetings);
		for (int i = 0; i < meetings.size(); ++i) {
			room.add_meeting(meetings[i]);
		}

		//room.print();
		//std::cout << "Number of meetings scheduled: " << room.meetings_scheduled() << "\n\n";

		etsd += room.meetings_scheduled();
		room.reset();

		// Shortest duration, earliest time
		//std::cout << "Shortest duration, earliest time\n";

		sort_meetings_sdet(meetings);
		for (int i = 0; i < meetings.size(); ++i) {
			room.add_meeting(meetings[i]);
		}

		//room.print();
		//std::cout << "Number of meetings scheduled: " << room.meetings_scheduled() << "\n\n";

		sdet += room.meetings_scheduled();
		room.reset();

		// Latest end, shortest duration
		//std::cout << "Latest end, shortest duration\n";

		sort_meetings_lesd(meetings);
		for (int i = 0; i < meetings.size(); ++i) {
			room.add_meeting(meetings[i]);
		}

		//room.print();
		//std::cout << "Number of meetings scheduled: " << room.meetings_scheduled() << "\n\n";

		lesd += room.meetings_scheduled();
		room.reset();

		// Latest end, longest duration
		//std::cout << "Latest end, longest duration\n";

		sort_meetings_leld(meetings);
		for (int i = 0; i < meetings.size(); ++i) {
			room.add_meeting(meetings[i]);
		}

		//room.print();
		//std::cout << "Number of meetings scheduled: " << room.meetings_scheduled() << "\n\n";

		leld += room.meetings_scheduled();
		room.reset();

		// Shortest duration, latest end
		//std::cout << "Shortest duration, latest end\n";

		sort_meetings_sdle(meetings);
		for (int i = 0; i < meetings.size(); ++i) {
			room.add_meeting(meetings[i]);
		}

		//room.print();
		//std::cout << "Number of meetings scheduled: " << room.meetings_scheduled() << "\n\n";
		
		sdle += room.meetings_scheduled();
		room.reset();

		// Longest duration, latest end
		//std::cout << "Longest duration, latest end\n";

		sort_meetings_ldle(meetings);
		for (int i = 0; i < meetings.size(); ++i) {
			room.add_meeting(meetings[i]);
		}

		//room.print();
		//std::cout << "Number of meetings scheduled: " << room.meetings_scheduled() << "\n\n";
		
		ldle += room.meetings_scheduled();
		room.reset();
	}
	auto process_end = std::chrono::high_resolution_clock::now();

	std::cout << "Average number of meetings scheduled per scheduling algorithm:" 
		<< "\n  Earliest time, shortest duration : " << (etsd / reps) 
		<< "\n  Shortest duration, earliest time: " << (sdet / reps) 
		<< "\n  Latest end, shortest duration: " << (lesd / reps) 
		<< "\n  Latest end, longest duration: " << (leld / reps) 
		<< "\n  Shortest duration, latest end: " << (sdle / reps) 
		<< "\n  Longest duration, latest end: " << (ldle / reps) << "\n";

	std::cout << "\nStats:" 
		<< "\n  starts before 12: " << start_before 
		<< "\n  starts at or after 12: " << start_after 
		<< "\n  avg dur : " << (total_duration / (start_before + start_after)) 
		<< "\n  time taken : " << std::chrono::duration_cast<std::chrono::milliseconds>(process_end - process_start) << "\n";
	std::cout << "\nTotal meeting scheduled per scheduling algorithm:" 
		<< "\n  etsd count : " << etsd 
		<< "\n  sdet count : " << sdet 
		<< "\n  lesd count : " << lesd 
		<< "\n  leld count : " << leld 
		<< "\n  sdle count : " << sdle 
		<< "\n  ldle count : " << ldle << "\n";
	return 0;
}