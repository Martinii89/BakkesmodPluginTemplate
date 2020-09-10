#pragma once
namespace ImGui {

	bool BeginTimeline(const char* str_id, float max_time);
	bool TimelineEvent(const char* str_id, float times[2]);
	void EndTimeline(float current_time = -1);

}

