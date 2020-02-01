#pragma once
#include <stdio.h>

class FlowControl
{
	private:
		enum Mode
		{
			Good,
			Bad
		};

		Mode mode;
		float penalty_time;
		float good_conditions_time;
		float penalty_reduction_accumulator;

	public:
		FlowControl();
		void Reset();
		void Update(float deltaTime, float rtt);
		float GetSendRate();
};