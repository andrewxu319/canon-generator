#include "settings.h"
#include "counterpoint_checker.h"

#include "mx/api/ScoreData.h"

#include <utility>
#include <iostream>
#include <cmath>

#define DEBUG // When defined, all errors will show. Encountering an error will not call return

void send_error_message(const Message& error, std::vector<Message>& error_message_box){
	for (Message& existing_error : error_message_box) {
	// Check if a similar error message already exists
		if (((existing_error.sonority_1_index) == (error.sonority_1_index)) && ((existing_error.sonority_2_index) == (error.sonority_2_index))) {
			return;
		}
	}

	error_message_box.emplace_back(error);
}

void send_warning_message(const Message& warning, std::vector<Message>& warning_message_box, std::vector<Message>& error_message_box) {
	for (Message& existing_warning : warning_message_box) {
		// Check if a similar warning OR error message already exists
		if (((existing_warning.sonority_1_index) == (warning.sonority_1_index)) && ((existing_warning.sonority_2_index) == (warning.sonority_2_index))) {
			return;
		}
	}
	for (Message& existing_error : error_message_box) {
		if (((existing_error.sonority_1_index) == (warning.sonority_1_index)) && ((existing_error.sonority_2_index) == (warning.sonority_2_index))) {
			return;
		}
	}

	warning_message_box.emplace_back(warning);
}


const int get_note_start_index(const int current_sonority_index, const int voice, const SonorityArray& sonority_array) {
	for (int i{ current_sonority_index - 1 }; i >= 0; --i) {
		if ((sonority_array.at(i).get_note_motion(voice).second != 0) || sonority_array.at(i).get_note(voice).isRest) {
			return i + 1;
			// Search until note isn't stationary or is a rest, then return the next note
		}
	}
	return current_sonority_index; // If no match, current sonority is the result
}

const int get_note_end_index(const int current_sonority_index, const int voice, const SonorityArray& sonority_array) {
	for (int i{ current_sonority_index }; i < sonority_array.size(); ++i) {
		if (sonority_array.at(i).get_note(voice).isRest) {
			return i - 1;
			// If encountering a rest, return the previous note
		}
		if (sonority_array.at(i).get_note_motion(voice).second != 0) {
			return i;
			// Search until note is stationary, then return that note
		}
	}
	return current_sonority_index; // If no match, current sonority is the result
}

const bool are_upbeat_parallels_legal(const SonorityArray& sonority_array, const std::vector<int>& index_array, const Sonority& current_sonority, const int note_1_index, const int note_2_index, const std::pair<std::vector<int>, std::vector<int>>& dissonant_intervals) {
	// Allow if there is a consonant suspension between the parallels
	//if (sonority_array)
	
	{
		for (int voice{ 0 }; voice < 2; ++voice) {
/* Not actually sure if this is a rule
			if (sonority_array.at(get_note_start_index(note_1_index, voice, sonority_array)).get_rhythmic_hierarchy() > sonority_array.at(note_2_index).get_rhythmic_hierarchy()) {
				// If note 1 starts on a strong(er) beat, consonant suspension doesn't make the parallels legal
				continue;
			}
*/

			int consonant_suspension_start{ -1 };
			for (int k{ note_1_index + 1 }; k < note_2_index; ++k) {
				// Find intervening downbeat
				if (sonority_array.at(index_array.at(k)).get_rhythmic_hierarchy() > current_sonority.get_rhythmic_hierarchy()) {
					consonant_suspension_start = index_array.at(k);
				}
			}
			if (consonant_suspension_start == -1) {
				// No valid downbeat found. Terminate loop for both voices.
				break;
			}

			const int consonant_suspension_end{ get_note_end_index(consonant_suspension_start, 0, sonority_array) };
			if (sonority_array.at(consonant_suspension_start).is_sonority_dissonant(dissonant_intervals) || sonority_array.at(consonant_suspension_end).is_sonority_dissonant(dissonant_intervals)) {
				// If consonant suspension isn't consonant, terminate loop for current voice
				continue;
			}
			bool note_1_is_held{ true };
			for (int k{ index_array.at(note_1_index) }; k < consonant_suspension_end; ++k) {
				// Loop through all notes between current_sonority and the the end of the consonant suspension to make sure the suspension is held thorugh
				if (sonority_array.at(k).get_note_motion(voice).second != 0) {
					note_1_is_held = false;
				}
			}
			if (!note_1_is_held) {
				continue;
			}
#ifdef DEBUG
			std::cout << "consonant suspension legalizes perfect parallels\n";
#endif // DEBUG
			return true;
		}
	}

	// Allow if each of the notes forming the parallel is approached from a different direction.
	for (int voice{ 0 }; voice < 2; ++voice) {
		try {
			if ((sonority_array.at(index_array.at(note_1_index) - 1).get_note_motion(voice).second * sonority_array.at(index_array.at(note_2_index) - 1).get_note_motion(voice).second < 0)
				|| (get_interval(sonority_array.at(index_array.at(note_1_index - 1)).get_note(voice), sonority_array.at(index_array.at(note_1_index)).get_note(voice), true).second
					* get_interval(sonority_array.at(index_array.at(note_2_index - 1)).get_note(voice), sonority_array.at(index_array.at(note_2_index)).get_note(voice), true).second
					< 0)
				// First line checks each voice, checks whether the notes before the perfect parallels (regardless of rhythmic hierarchy) approach from opposite directions
				// Other lines do the same but for notes of equal or greater rhythmic hierarchy immediately before the sonorities comprising the perfect intervals
				)
			{
				return true;
			}
		}
		catch(...){}
	}

	return false;
}

void check_voice_independence(const SonorityArray& sonority_array, std::vector<int> index_array, const std::pair<std::vector<int>, std::vector<int>>& dissonant_intervals, std::vector<Message>& error_message_box, std::vector<Message>& warning_message_box, const int ticks_per_measure, const int rhythmic_hierarchy_max_depth, const int rhythmic_hierarchy_of_beat, const std::size_t voice_count, const ScaleDegree& leading_tone) {
	for (int i{ 0 }; i < index_array.size() - 1; ++i) { // Subtract 1 because we don't want to check the last sonority
		const Sonority& current_sonority{ sonority_array.at(index_array.at(i)) };
		const Sonority& next_sonority{ (sonority_array).at(index_array.at(i + 1)) };

		if (current_sonority.get_note_1().isRest ||
			current_sonority.get_note_2().isRest ||
			next_sonority.get_note_1().isRest ||
			next_sonority.get_note_2().isRest)
		{
			continue;
		}


		// Avoid doubled leading tone
		if ((current_sonority.get_note_1().pitchData.step == leading_tone.step && current_sonority.get_note_1().pitchData.alter == leading_tone.alter)
			&& (current_sonority.get_note_2().pitchData.step == leading_tone.step && current_sonority.get_note_2().pitchData.alter == leading_tone.alter))
		{
			send_warning_message(Message{ "Doubled leading tone", current_sonority.get_index(), current_sonority.get_index() }, warning_message_box, error_message_box);
		}

		// Avoid following a perfect consonance with another one.
		if ((current_sonority.get_simple_interval().second == 7 || current_sonority.get_simple_interval().second == 0)
			&& (next_sonority.get_simple_interval().second == 7 || next_sonority.get_simple_interval().second == 0))
		{
			send_warning_message(Message{ "Consecutive perfect consonances", current_sonority.get_index(), next_sonority.get_index() }, warning_message_box, error_message_box);
		}

		// Avoid similar motion from a 2nd to a 3rd
		// Remove? Warning?
		if (current_sonority.get_motion_type() == similar
			&& ((current_sonority.get_simple_interval().first == 1
				&& next_sonority.get_simple_interval().first == 2)
				// 2nd to 3rd
				|| (current_sonority.get_simple_interval().first == 6
					&& next_sonority.get_simple_interval().first == 5)))
			// Inverted---7th to 6th
		{
			send_warning_message(Message{ "Similar motion from a 2nd to a 3rd", current_sonority.get_index(), next_sonority.get_index() }, warning_message_box, error_message_box);
		}

		// Avoid more than 3 successive uses of the same interval in the same voices
		try {
			if (current_sonority.get_signed_compound_interval().first == next_sonority.get_signed_compound_interval().first
				&& current_sonority.get_signed_compound_interval().first == sonority_array.at(index_array.at(i + 2)).get_signed_compound_interval().first
				// If 3 successive uses of the same interval in the same voices
				&& (i == 0 || current_sonority.get_compound_interval().first != sonority_array.at(index_array.at(i - 1)).get_compound_interval().first)
				// If previous sonority isn't the same interval---ensures no duplicate warnings are raised if parallels continue for longer (CAN COMMENT THIS OUT IF WE WANT MULTIPLE WARNINGS)
				)
			{
				if (voice_count > 2) {
					// Allowed in 3+ parts
					send_warning_message(Message{ "Parallel intervals for 4 or more notes (might continue beyond indicated final note)", current_sonority.get_index(), sonority_array.at(index_array.at(i + 2)).get_index() }, warning_message_box, error_message_box);
				}
				else {
					send_error_message(Message{ "Parallel intervals for 4 or more notes (might continue beyond indicated final note)", current_sonority.get_index(), sonority_array.at(index_array.at(i + 2)).get_index() }, error_message_box);
#ifndef DEBUG
					return;
#endif
				}
			}
		}
		catch (...) {}

		// Allow parallels if the faster voice leaps by more than a third from the first perfect interval. Overrides all other rules for parallels
		bool parallels_allowed{ false };
		if (index_array.at(i + 1) - index_array.at(i) >= 2) {
			// There needs to be a gap between the two perfect consonances
			for (int voice{ 0 }; voice < 2; ++voice) {
				const int other_voice{ (voice == 0) ? 1 : 0 };

				for (int j{ index_array.at(i) }; j < index_array.at(i + 1) - 1; ++j)
					// For each intervening note
				{
					if ((std::abs(sonority_array.at(j).get_note_motion(voice).first) > 2)
						// Leap of more than a third
						&& (sonority_array.at(j).get_note_motion(other_voice).second == 0)
						// Other voice cannot move
						) {
#ifdef DEBUG
						std::cout << "Parallels allowed: faster voice leaps by more than a third from the first perfect interval\n";
#endif // DEBUG
						parallels_allowed = true;
					}
				}
			}
		}
		if (parallels_allowed) {
			continue;
		}

		// Avoid parallel fifths and octaves between adjacent notes.
		if (current_sonority.get_simple_interval().second == 7 && next_sonority.get_simple_interval().second == 7) {
			send_error_message(Message{ "Parallel fifths between adjacent notes or downbeats", current_sonority.get_index(), next_sonority.get_index() }, error_message_box);
#ifndef DEBUG
			return;
#endif
		} else
		if (current_sonority.get_simple_interval().first == 0 && next_sonority.get_simple_interval().first == 0) {

			send_error_message(Message{ "Parallel octaves between adjacent notes or downbeats", current_sonority.get_index(), next_sonority.get_index() }, error_message_box);
#ifndef DEBUG
			return;
#endif
		}

		// Avoid parallel fifths and octaves between any part of a beat and an accented note on the next beat.
		std::vector<int> checked_rhythmic_levels{}; // Only get FIRST of any given downbeat level
		if ((i == 0 || (sonority_array.at(index_array.at(i - 1)).get_note_1_motion().second != 0 && sonority_array.at(index_array.at(i - 1)).get_note_2_motion().second != 0))
			// Allow if the first notes don't begin simultaneously
			&& (current_sonority.get_rhythmic_hierarchy() >= rhythmic_hierarchy_of_beat))
			// Allow if both first notes are offbeat
		{
			for (int j{ i + 1 };
				j < (index_array.size())
				&& (sonority_array.at(index_array.at(j)).get_index() - current_sonority.get_index() <= ticks_per_measure) // Search up to the end of the measure
				&& (checked_rhythmic_levels.size() < (rhythmic_hierarchy_max_depth - current_sonority.get_rhythmic_hierarchy() - 1)); // End search when all rhythmic levels are found
				++j) {
				const Sonority& downbeat{ sonority_array.at(index_array.at(j)) };

				if (downbeat.get_rhythmic_hierarchy() > current_sonority.get_rhythmic_hierarchy()
					// If hierarchy level hasn't been checked yet
					&& find(checked_rhythmic_levels.begin(), checked_rhythmic_levels.end(), downbeat.get_rhythmic_hierarchy()) == checked_rhythmic_levels.end()
					&& !downbeat.is_sonority_dissonant(dissonant_intervals))
				{

					if (current_sonority.get_simple_interval().second == 7 && downbeat.get_simple_interval().second == 7) {
						send_error_message(Message{ "Parallel fifths between weak beat and downbeat", current_sonority.get_index(), downbeat.get_index() }, error_message_box);
						#ifndef DEBUG
						return;
						#endif
						checked_rhythmic_levels.push_back(downbeat.get_rhythmic_hierarchy());
					}
					else
						if (current_sonority.get_simple_interval().first == 0 && downbeat.get_simple_interval().first == 0) {
							send_error_message(Message{ "Parallel octaves between weak beat and downbeat", current_sonority.get_index(), downbeat.get_index() }, error_message_box);
							#ifndef DEBUG
							return;
							#endif
							checked_rhythmic_levels.push_back(downbeat.get_rhythmic_hierarchy());
						}

					if (!downbeat.is_sonority_dissonant(dissonant_intervals)) {
						// Allow if intervening notes (current note is an intervening note for all future notes) are concords.
						break;
					}
				}
			}
		}

		// Avoid parallel fifths and octaves between notes following adjacent accents, if voices are 2:1
		for (int j{ i + 1 }; j < index_array.size(); ++j) {
			const Sonority& next_upbeat{ sonority_array.at(index_array.at(j)) };

			// Unless the gap between them is a measure or more (arbitrary line I drew)
			if (next_upbeat.get_index() - current_sonority.get_index() >= ticks_per_measure) {
				continue;
			}

			if (next_upbeat.get_rhythmic_hierarchy() == current_sonority.get_rhythmic_hierarchy()) {
				if (current_sonority.get_simple_interval().second == 7 && next_upbeat.get_simple_interval().second == 7) {
					if (!are_upbeat_parallels_legal(sonority_array, index_array, current_sonority, i, j, dissonant_intervals)) {
						send_error_message(Message{ "Parallel fifths between consecutive upbeats", current_sonority.get_index(), next_upbeat.get_index() }, error_message_box);
#ifndef DEBUG
						return;
#endif
					}
				}
				else
					if (current_sonority.get_simple_interval().first == 0 && next_upbeat.get_simple_interval().first == 0) {
						if (!are_upbeat_parallels_legal(sonority_array, index_array, current_sonority, i, j, dissonant_intervals)) {
							send_error_message(Message{ "Parallel octaves between consecutive upbeats", current_sonority.get_index(), next_upbeat.get_index() }, error_message_box);
#ifndef DEBUG
							return;
#endif
						}
					}
				break;
			}
		}
	}
}

const int max_rhythmic_hierarchy(const int first_note_start_sonority_index, const int second_note_sonority_index, const std::vector<int>& rhythmic_hierarchy_array) {
	int max{ 0 };
	for (int i{ first_note_start_sonority_index }; i < second_note_sonority_index; ++i) {
		int max_at_i{ rhythmic_hierarchy_array.at(i % rhythmic_hierarchy_array.size()) };
		if (max_at_i > max) {
			max = max_at_i;
		}
	}
	return max;
}

void is_dissonance_allowed(const SonorityArray& sonority_array, const int i, const std::pair<std::vector<int>, std::vector<int>>& dissonant_intervals, std::vector<bool>& allowed_dissonances, const int ticks_per_measure, const ScaleDegree& tonic, const ScaleDegree& dominant, const ScaleDegree& leading_tone, const std::vector<int>& rhythmic_hierarchy_array, std::vector<Message>& error_message_box, std::vector<Message>& warning_message_box) {
	// Decide whether to pass these directly
	const Sonority& current_sonority{ sonority_array.at(i) };
	const Sonority& next_sonority{ sonority_array.at(i + 1) };

	// PEDAL TONE
	for (int voice{ 0 }; voice < 2; ++voice) {
		try {
			const int pedal_start{ get_note_start_index(i, voice, sonority_array) };
			const int pedal_end{ get_note_end_index(i, voice, sonority_array) };

			if ((pedal_end - pedal_start >= 6)
				// A pedal will be defined as a note that lasts (or repeats) through more than six pitch changes in other voices. Really it would be better to define it as a tone that is held through two or more changes of harmony, but for technical reasons we'll make this arbitrary definition.
				&& ((sonority_array.at(pedal_start).get_note(voice).pitchData.step == tonic.step)
					// Pedal can be tonic
					|| (sonority_array.at(pedal_start).get_note(voice).pitchData.step == dominant.step))
				// Or dominant
				&& (!sonority_array.at(pedal_start).is_sonority_dissonant(dissonant_intervals) && !sonority_array.at(pedal_end).is_sonority_dissonant(dissonant_intervals))
				// Start and end must be consonant
				)
			{
				for (int j{ pedal_start }; j <= pedal_end; ++j) {
					allowed_dissonances.at(j) = true;
				}
#ifdef DEBUG
				std::cout << "pedal tone\n";
#endif // DEBUG
				return;
			}
		}
		catch (...) {}
	}

	for (int voice{ 0 }; voice < 2; ++voice) {
		// Allow 2 successive dissonances if they are quick and stepwise.
		// Here we define "quick" as there are no other voices moving faster
		try {
			if ((!sonority_array.at(i - 1).is_sonority_dissonant(dissonant_intervals))
				// Note 1 must be consonant
				&& (!sonority_array.at(i + 2).is_sonority_dissonant(dissonant_intervals))
				// Note 4 must be consonant
				)
			{
				if ((sonority_array.at(i - 1).get_note_motion(voice).first == 1 && current_sonority.get_note_motion(voice).first == 1 && next_sonority.get_note_motion(voice).first == 1) ||
					(sonority_array.at(i - 1).get_note_motion(voice).first == -1 && current_sonority.get_note_motion(voice).first == -1 && next_sonority.get_note_motion(voice).first == -1))
					// All notes must move stepwise in the same direction
				{
#ifdef DEBUG
					std::cout << "double passing tones\n";
#endif // DEBUG
					allowed_dissonances.at(i) = true;
					allowed_dissonances.at(i + 1) = true;
					return;
				}
				if ((sonority_array.at(i - 1).get_note_motion(voice).first == -1)
					// Note 1 descends stepwise
					&& (current_sonority.get_note_motion(voice).first == -1)
					// Note 2 descends stepwise
					&& (next_sonority.get_note_motion(voice).first == 0)
					// Note 3 stays stationary
					&& (sonority_array.at(i + 2).get_rhythmic_hierarchy() > next_sonority.get_rhythmic_hierarchy())
					// Note 3 must be on upbeat
					&& ((sonority_array.at(i + 2).get_index() - next_sonority.get_index()) <= (next_sonority.get_index() - current_sonority.get_index()))
					// Note 3 cannot be longer than note 2
					)
				{
#ifdef DEBUG
					std::cout << "anticipation with two dissonances\n";
#endif // DEBUG
					allowed_dissonances.at(i) = true;
					allowed_dissonances.at(i + 1) = true;
					return;
				}
			}
		}
		catch (...) {}
	}

	// CAMBIATA, DOUBLE NEIGHBOR NOTE (and all other four-note sequences)
	for (int voice{ 0 }; voice < 2; ++voice) {
		try {
			// "Try" ensures there are enough notes on either side
			const int note_2_start{ get_note_start_index(i, voice, sonority_array) };
			const int note_2_end{ get_note_end_index(i, voice, sonority_array) };
			const int note_1_start{ get_note_start_index(note_2_start - 1, voice, sonority_array) };
			const int note_1_end{ note_2_start - 1 };
			const int note_3_start{ note_2_end + 1 };
			const int note_3_end{ get_note_end_index(note_3_start, voice, sonority_array) };
			const int note_4_start{ note_3_end + 1 };
			//const int note_4_end{ get_note_end_index(note_4_start, 1, sonority_array) }; // Unused

// FIX: SKIP OVER IDENTICAL NOTES OR CONSONANT INTERJECTIONS. SIMILAR FIX AS ANTICIPATIONS  https://en.wikipedia.org/wiki/Cambiata#:~:text=Generally%20it%20refers%20to%20a,the%20opposite%20direction%2C%20and%20where
// OTHER VOICE CAN'T LEAP??
// Allow leap to dissonance in cambiata. Allow double auxiliary.
// Current sonority = second note
			if ((!sonority_array.at(note_1_end).is_sonority_dissonant(dissonant_intervals))
				// Note 1 must be consonant
				&& ((sonority_array.at(note_3_start).get_index() - sonority_array.at(note_2_start).get_index()) <= (sonority_array.at(note_2_start).get_index() - sonority_array.at(note_1_start).get_index()) && (sonority_array.at(note_3_start).get_index() - sonority_array.at(note_2_start).get_index()) <= (sonority_array.at(note_4_start).get_index() - sonority_array.at(note_3_start).get_index()))
				// Second note cannot be longer than notes 1 and 3
				&& (current_sonority.get_rhythmic_hierarchy() <= max_rhythmic_hierarchy(sonority_array.at(note_1_start).get_index(), sonority_array.at(note_2_start).get_index(), rhythmic_hierarchy_array) && current_sonority.get_rhythmic_hierarchy() <= sonority_array.at(note_3_start).get_rhythmic_hierarchy())
				// Second note must be on weak beat
				)
			{
				if ((sonority_array.at(note_1_end).get_note_motion(voice).first == -1)
					&& (sonority_array.at(note_2_end).get_note_motion(voice).first == -2)
					&& (sonority_array.at(note_3_end).get_note_motion(voice).first == 1)) {
					// If the voice moves correctly (downward cambiata)
#ifdef DEBUG
					std::cout << "downward cambiata\n";
#endif // DEBUG
					for (int j{ i }; j <= note_3_end; ++j) {
						allowed_dissonances.at(j) = true;
					}
					return;
				}
				else
				if ((sonority_array.at(note_1_end).get_note_motion(voice).first == 1)
					&& (sonority_array.at(note_2_end).get_note_motion(voice).first == 2)
					&& (sonority_array.at(note_3_end).get_note_motion(voice).first == -1)) {
					// If the voice moves correctly (inverted upward cambiata)
#ifdef DEBUG
					std::cout << "inverted (upward) cambiata\n";
#endif // DEBUG
					for (int j{ i }; j <= note_3_end; ++j) {
						allowed_dissonances.at(j) = true;
					}
					return;
				} else
				if (((sonority_array.at(note_1_end).get_note_motion(voice).first == -1)
					&& (sonority_array.at(note_2_end).get_note_motion(voice).first == 2)
					&& (sonority_array.at(note_3_end).get_note_motion(voice).first == -1)) ||
					// Down-up double neighbor tone
					((sonority_array.at(note_1_end).get_note_motion(voice).first == 1)
						&& (sonority_array.at(note_2_end).get_note_motion(voice).first == -2)
						&& (sonority_array.at(note_3_end).get_note_motion(voice).first == 1))) {
					// Up-down double neighbor tone
#ifdef DEBUG
					std::cout << "double neighbor tone\n";
#endif // DEBUG
					for (int j{ i }; j <= note_3_end; ++j) {
						allowed_dissonances.at(j) = true;
					}
					return;
				}
			}
		}
		catch (...) {}
	}

	// Here we've dealt with all multi-dissonance sequences. Prohibit all other consecutive dissonances
	if (next_sonority.is_sonority_dissonant(dissonant_intervals)) {
#ifdef DEBUG
		std::cout << "multiple consecutive dissonances error\n";
#endif // DEBUG
		allowed_dissonances.at(i) = false;
		return;
	}

	for (int voice{ 0 }; voice < 2; ++voice) {
		// PASSING & NEIGHBOR NOTE
		try {
			if ((!next_sonority.is_sonority_dissonant(dissonant_intervals))
				// Resolve dissonance immediately by step. (Resolution is always the next sonority)
				&& (std::abs(sonority_array.at(i - 1).get_note_motion(voice).first) == 1)
				// If prepared by step
				&& (std::abs((current_sonority.get_note_motion(voice).first)) == 1)
				// If resolves by step
				&& (!sonority_array.at(i - 1).is_sonority_dissonant(dissonant_intervals))
				// If preparation is consonant
				)
			{
#ifdef DEBUG
				std::cout << "passing/neighbor note\n";
#endif // DEBUG
				allowed_dissonances.at(i) = true;
				return;
			}
		}
		catch (...) {}

		// SUSPENSION
		try {
			if (sonority_array.at(i - 1).get_note_motion(voice).second == 0
				// If preparation is held into the dissonance
				&& (current_sonority.get_rhythmic_hierarchy() > next_sonority.get_rhythmic_hierarchy())
				// The dissonance must occur on a strong beat (1 or 3)
				&& (current_sonority.get_index() - sonority_array.at(get_note_start_index(i - 1, voice, sonority_array)).get_index() >= next_sonority.get_index() - current_sonority.get_index())
				// Require preparation to be equal to or longer than dissonance.
				)
			{
				for (int j{ i + 1 }; j < sonority_array.size(); ++j) {
					const Sonority& resolution{ sonority_array.at(j) };
					if (resolution.get_rhythmic_hierarchy() >= current_sonority.get_rhythmic_hierarchy()) {
						// Stop search upon hitting a stronger or equal beat
						break; // No appogiatura found, go to next check
					}

					// LEGAL RETARDATION
					if (current_sonority.get_note(voice).pitchData.step == leading_tone.step && current_sonority.get_note(voice).pitchData.alter == leading_tone.alter
						&& resolution.get_note(voice).pitchData.step == tonic.step && resolution.get_note(voice).pitchData.alter == tonic.alter) {
						// If retardation resolves from leading tone to tonic

						if (resolution.get_simple_interval().first == 0) {
							// Can't really resolve to a perfect 5th since other voice can't move
							send_warning_message(Message{ "Retardation resolution is doubled!", current_sonority.get_index(), resolution.get_index() }, warning_message_box, error_message_box);
						}
						else {
#ifdef DEBUG
							std::cout << "retardation resolves up by semitone\n";
#endif // DEBUG
							allowed_dissonances.at(i) = true;
							return;
						}
					}

/* commenting this out because we want to check all other conditions
					// ILLEGAL RETARDATION
					if (get_interval(current_sonority.get_note(voice), resolution.get_note(voice), true).second > 1) {
						send_error_message(Message{ "Illegal retardation!", current_sonority.get_index(), resolution.get_index() }, error_message_box);
						#ifndef DEBUG
						return;
						#endif
						allowed_dissonances.at(i) = true; // Not actually allowed but we don't want it to complain again
						return;
					}
*/

					// PROPER DOWNWARD SUSPENSION
					if (get_interval(current_sonority.get_note(voice), resolution.get_note(voice), true).first == -1)
						// Moves by step downwards into resolution
					{
						if ((current_sonority.get_simple_interval().first == 6) && (resolution.get_simple_interval().first == 0))
							// Prohibit 7-8 suspensions
						{
							send_error_message(Message{ "7-8 suspension", current_sonority.get_index(), resolution.get_index() }, error_message_box);
						}
						else if (resolution.get_simple_interval().first == 0 || resolution.get_simple_interval().second == 7) {
							send_warning_message(Message{ "Suspension resolves to perfect consonance", current_sonority.get_index(), resolution.get_index() }, warning_message_box, error_message_box);
						}

#ifdef DEBUG
						std::cout << "suspension\n";
#endif // DEBUG
						allowed_dissonances.at(i) = true;
						return;
					}
				}
			}
		}
		catch (...) {}

		// APPOGIATURA
		try {
			for (int j{ i + 1 }; j < sonority_array.size(); ++j) {
				const Sonority& resolution{ sonority_array.at(j) };
				if (resolution.get_index() / ticks_per_measure > current_sonority.get_index() / ticks_per_measure) {
					// Stop search at end of measure
					break; // No appogiatura found, go to next check
				}

				const int other_voice{ (voice == 0) ? 1 : 0 };
				if ((get_interval(current_sonority.get_note(voice), resolution.get_note(voice), false).first == 1)
					// Moves by step into resolution
					&& (get_interval(current_sonority.get_note(other_voice), resolution.get_note(other_voice), true).second == 0)
					// Other voice must be the same at resolution
					)
				{
					if ((sonority_array.at(i - 1).get_note(voice).isRest)
						// If preparation is rest
						|| (sonority_array.at(i - 1).get_note_motion(voice).second * get_interval(current_sonority.get_note(voice), resolution.get_note(voice), true).second < 0)
						// If preparation leaps in opposite direction
						)
					{
						if (resolution.get_simple_interval().first == 0) {
							const int lower_voice{ (current_sonority.get_signed_compound_interval().second > 0) ? 0 : 1 };
							if ((current_sonority.get_simple_interval().first == 6) && (resolution.get_simple_interval().first == 0)
								// Prohibit 7-8 appogiaturas in the lower voice
								&& (get_interval(current_sonority.get_note(lower_voice), next_sonority.get_note(lower_voice), true).second != 0)
								// Lower voice moves (signed doesn't matter, signed interval is cheaper)
								)
							{
								send_error_message(Message{ "7-8 appogiatura", current_sonority.get_index(), resolution.get_index() }, error_message_box);
							}
							else {
								// Can't really resolve to a perfect 5th since other voice can't move
								send_warning_message(Message{ "Appogiatura resolution is doubled!", current_sonority.get_index(), resolution.get_index() }, warning_message_box, error_message_box);
							}
						}
#ifdef DEBUG
						std::cout << "appogiatura\n";
#endif // DEBUG
						allowed_dissonances.at(i) = true;
						return;
					}
					else if (std::abs(sonority_array.at(i - 1).get_note_motion(voice).second) > 1) {
						// "APPOGIATURA" from a leap of the SAME DIRECTION---warning
						// Preparation to dissonance can't be stationary because that would have been resolved by the suspension check
						
						//if (allowed_dissonances.at(i) == false) {
						//	// Ensures something marked as the third note of a cambiata/double neighbor tone sequence isn't checked 
							send_warning_message(Message{ "Appogiatura leaps in from the same direction!", sonority_array.at(i - 1).get_index(), current_sonority.get_index() }, warning_message_box, error_message_box);
						//}
						allowed_dissonances.at(i) = true;
						return;
					}
				}
			}
		}
		catch (...) {}
	}

	// MAYBE ADD MORE CONDITIONS. (anticipation must be shorter than preparation, other voice can move during anticipation), Allow successive dissonance if the second tone is an anticipation
	// Allow unaccented leap to anticipation.
	for (int voice{ 0 }; voice < 2; ++voice) {
		const int other_voice{ (voice == 0) ? 1 : 0 };
		const int preparation_start{ get_note_start_index(i - 1, voice, sonority_array) };
		try {
			if ((sonority_array.at(i - 1).get_note_motion(other_voice).second == 0 && current_sonority.get_note_motion(voice).second == 0)
				// If one voice is oblique, then the next
				// Other voice can't move during anticipation. In multi-voice textures, one voice might have an anticipation in relation to another slower-moving voice, but have non-anticipation counterpoint in relation to another faster-moving voice
				&& (sonority_array.at(i - 1).get_note_motion(voice).second < 0)
				// Always uses descending motion, never ascending motion
				&& (current_sonority.get_rhythmic_hierarchy() < next_sonority.get_rhythmic_hierarchy() && current_sonority.get_rhythmic_hierarchy() <= sonority_array.at(preparation_start).get_rhythmic_hierarchy())
				// If dissonance is on upbeat with relation to the preparation and at least the same hierarchy as the resolution
				&& ((next_sonority.get_index() - current_sonority.get_index()) <= (current_sonority.get_index() - sonority_array.at(preparation_start).get_index()))
				// Preparation must be at least the same length as the dissonance
				)
			{
#ifdef DEBUG
				std::cout << "anticipation\n";
#endif // DEBUG
				allowed_dissonances.at(i) = true;
				return;
			}
		}
		catch (...){}
	}

// Allow suspension. https://www.youtube.com/watch?v=g-4dw1T3v30
// 7-8 suspension is a warning
	/*
	* Search for preparation
	* Preparation must be consonant
	* IF RESOLUTION NOT DELAYED---make all int
	* Attempt to find resolution 
	*/

// ESCAPE TONE
	for (int voice{ 0 }; voice < 2; ++voice) {
		try {
			const int preparation_start{ get_note_start_index(i - 1, voice, sonority_array) };
			const int preparation_end{ i - 1 };
			// I don't think the other voice can move during the escape tone

			if ((std::abs(sonority_array.at(preparation_end).get_note_motion(voice).first) <= 1)
				// Preparation moves by step
				&& (!sonority_array.at(preparation_end).is_sonority_dissonant(dissonant_intervals))
				// End of preparation must be consonant
				&& (current_sonority.get_note(voice).durationData.durationTimeTicks <= current_sonority.get_index() - sonority_array.at(preparation_start).get_index())
				// Preparation must be longer than or equal in length to escape tone
				&& (current_sonority.get_rhythmic_hierarchy() <= sonority_array.at(preparation_start).get_rhythmic_hierarchy() && current_sonority.get_rhythmic_hierarchy() <= next_sonority.get_rhythmic_hierarchy())
				// Preparation must be on weak beat / upbeat
				&& (sonority_array.at(i - 1).get_note_motion(voice).second * current_sonority.get_note_motion(voice).second < 0)
				// Must resolve in opposite direction
				) {
#ifdef DEBUG
				std::cout << "escape tone\n";
#endif // DEBUG
				allowed_dissonances.at(i) = true;
				return;
			}
		}
		catch (...) {}
	}

	// If no rule explicitly says it's legal, use whatever value is already stored
}

void check_dissonance_handling(const std::pair<std::vector<int>, std::vector<int>>& dissonant_intervals, const SonorityArray& sonority_array, std::vector<Message>& error_message_box, std::vector<Message>& warning_message_box, std::vector<bool>& is_tick_dissonance_start, const bool write_to_is_tick_dissonance_start, const int ticks_per_measure, const ScaleDegree& tonic, const ScaleDegree& dominant, const ScaleDegree& leading_tone, const std::vector<int>& rhythmic_hierarchy_array) {
	// TODO: last sonority cannot be dissonant

	std::vector<bool> allowed_dissonances(sonority_array.size());
	// False: not allowed. True: allowed / warning
	
	for (int i{ 0 }; i < sonority_array.size() - 1; ++i) { // Subtract 1 because we don't want to check the last sonority

		const Sonority& current_sonority{ sonority_array.at(i) };
		const Sonority& next_sonority{ (sonority_array).at(i + 1) };

		if (current_sonority.is_sonority_dissonant(dissonant_intervals)) {
			const int current_sonority_index{ current_sonority.get_index() };
			/*if (is_tick_dissonance_start.at(current_sonority_index)) {
				send_error_message(Message{ "Simultaneous dissonances", current_sonority_index, current_sonority_index }, error_message_box);
			}*/
			if (write_to_is_tick_dissonance_start) {
				is_tick_dissonance_start.at(current_sonority_index) = true;
			}
			is_dissonance_allowed(sonority_array, i, dissonant_intervals, allowed_dissonances, ticks_per_measure, tonic, dominant, leading_tone, rhythmic_hierarchy_array, error_message_box, warning_message_box);
				// If not allowed, is_dissonance_allowed will mark it as so
		}
		else {
			allowed_dissonances.at(i) = true;
		}
	}

	// Last sonority
	{
		const std::size_t i{ sonority_array.size() - 1 };
		if (sonority_array.at(i).is_sonority_dissonant(dissonant_intervals)) {
			allowed_dissonances.at(i) = false;
		}
		else {
			allowed_dissonances.at(i) = true;
		}
	}

	for (int i{ 0 }; i < allowed_dissonances.size(); ++i) {
		if (allowed_dissonances.at(i) == false) {
			send_error_message(Message{ "Illegal dissonance", sonority_array.at(i).get_index(), sonority_array.at(i).get_index() }, error_message_box);
			#ifndef DEBUG
			return;
			#endif
		}
	}
}

void check_with_given_config(const std::pair<std::vector<int>, std::vector<int>>& dissonant_intervals, std::vector<Message>& error_message_box, std::vector<Message>& warning_message_box, const std::vector<std::vector<int>>& index_arrays_for_sonority_arrays, const SonorityArray& stripped_sonority_array, std::vector<bool>& is_tick_dissonance_start, const bool write_to_is_tick_dissonance_start, const int ticks_per_measure, const ScaleDegree& tonic, const ScaleDegree& dominant, const ScaleDegree& leading_tone, const std::vector<int>& rhythmic_hierarchy_array, const int rhythmic_hierarchy_max_depth, const int rhythmic_hierarchy_of_beat, const std::size_t voice_count) {
	for (std::vector<int> index_array : index_arrays_for_sonority_arrays) {
		if (index_array.size() > 0) {
			check_voice_independence(stripped_sonority_array, index_array, dissonant_intervals, error_message_box, warning_message_box, ticks_per_measure, rhythmic_hierarchy_max_depth, rhythmic_hierarchy_of_beat, voice_count, leading_tone);
		}
	}
	check_dissonance_handling(dissonant_intervals, stripped_sonority_array, error_message_box, warning_message_box, is_tick_dissonance_start, write_to_is_tick_dissonance_start, ticks_per_measure, tonic, dominant, leading_tone, rhythmic_hierarchy_array); // Only check lowest level
}

void check_outer_voice(const SonorityArray& sonority_array, std::vector<int> index_array, const int outer_voice, std::vector<Message>& error_message_box, std::vector<Message>& warning_message_box) {
	const int inner_voice{ (outer_voice == 0) ? 1 : 0 };

	for (int i{ 0 }; i < index_array.size() - 1; ++i) { // Subtract 1 because we don't want to check the last sonority
		const Sonority& current_sonority{ sonority_array.at(index_array.at(i)) };
		const Sonority& next_sonority{ (sonority_array).at(index_array.at(i + 1)) };

	// Avoid "direct"or "hidden" 5ths or 8vas (similar motion to those intervals).
		if ((get_interval(current_sonority.get_note_1(), next_sonority.get_note_1(), true).second * get_interval(current_sonority.get_note_2(), next_sonority.get_note_2(), true).second > 0)
			// If similar motion
			&& (next_sonority.get_simple_interval().second == 7 || next_sonority.get_simple_interval().second == 0)
			// If next sonority is a perfect consonance
			)
		{
			if (!(std::abs(current_sonority.get_note_motion(outer_voice).first) <= 1))
				// Allow if one voice is inner and exposed voice moves by step
			{
				send_warning_message(Message{ "Direct fifths or octaves between outer and inner voice", current_sonority.get_index(), next_sonority.get_index() }, warning_message_box, error_message_box);
				// Error or warning?
			}
#ifdef DEBUG
			else {
				std::cout << "Direct fifths allowed between outer and inner voice: one voice is inner and exposed voice moves by step\n";
			}
#endif // DEBUG

		}

	// Avoid leaping motion in two voices moving to a perfect interval. ONLY IF INNER PART
		if (std::abs(current_sonority.get_note_1_motion().first) != 1 &&
			std::abs(current_sonority.get_note_2_motion().first) != 1) {
			if (next_sonority.get_simple_interval().second == 7) {
				send_warning_message(Message{ "Both voices leap to a perfect fifth", current_sonority.get_index(), next_sonority.get_index() }, warning_message_box, error_message_box);
#ifndef DEBUG
				return;
#endif 
			}
		else
			if (next_sonority.get_simple_interval().second == 0) {
				send_warning_message(Message{ "Both voices leap to an octave", current_sonority.get_index(), next_sonority.get_index() }, warning_message_box, error_message_box);
#ifndef DEBUG
				return;
#endif
			}
		}
	}
}

void check_outer_voice_pair(const SonorityArray& sonority_array, std::vector<int> index_array, std::vector<Message>& error_message_box, std::vector<Message>& warning_message_box, const int voice_count) {
	for (int i{ 0 }; i < index_array.size() - 1; ++i) {
		const Sonority& current_sonority{ sonority_array.at(index_array.at(i)) };
		const Sonority& next_sonority{ (sonority_array).at(index_array.at(i + 1)) };

		const int upper_voice{ (current_sonority.get_signed_compound_interval().second < 0) ? 0 : 1 };

		if ((get_interval(current_sonority.get_note_1(), next_sonority.get_note_1(), true).second * get_interval(current_sonority.get_note_2(), next_sonority.get_note_2(), true).second > 0))
			// If similar motion
		{
			if (next_sonority.get_simple_interval().second == 0)
				// Direct octaves
			{
				if ((!(std::abs(current_sonority.get_note_motion(upper_voice).first) <= 1))
					// Allow if upper voice moves by step
					|| (voice_count > 3)
					// And 4 or more voices
					)
				{
					if ((voice_count == 2)
						// If two voices
						|| (current_sonority.get_rhythmic_hierarchy() < next_sonority.get_rhythmic_hierarchy())
						// Second interval is on downbeat
						) {
						send_error_message(Message{ "Direct octaves between outer voices", current_sonority.get_index(), next_sonority.get_index() }, error_message_box);
					}
					else {
						send_warning_message(Message{ "Direct octaves between outer voices", current_sonority.get_index(), next_sonority.get_index() }, warning_message_box, error_message_box);
					}
				}
#ifdef DEBUG
				else {
					std::cout << "Direct octaves allowed between outer voices: 4 or more voices, upper voice moves by step\n";
				}
#endif // DEBUG
			}

			if (next_sonority.get_simple_interval().second == 7) {
				// Direct fifths
				if (!(std::abs(current_sonority.get_note_motion(upper_voice).first) <= 1))
					// Allow if upper voice moves by step
				{
					if ((voice_count == 2)
						// If two voices
						|| (current_sonority.get_rhythmic_hierarchy() < next_sonority.get_rhythmic_hierarchy())
						// Second interval is on downbeat
						) {
						send_error_message(Message{ "Direct fifths between outer voices", current_sonority.get_index(), next_sonority.get_index() }, error_message_box);
					}
					else {
						send_warning_message(Message{ "Direct fifths between outer voices", current_sonority.get_index(), next_sonority.get_index() }, warning_message_box, error_message_box);
					}
				}
#ifdef DEBUG
				else {
					std::cout << "Direct fifths allowed between outer voices: upper voice moves by step\n";
				}
#endif // DEBUG
			}
		}
	}
}

void check_counterpoint(Canon& canon, const int ticks_per_measure, const ScaleDegree& tonic, const ScaleDegree& dominant, const ScaleDegree& leading_tone, const std::vector<int>& rhythmic_hierarchy_array, const int rhythmic_hierarchy_max_depth, const int rhythmic_hierarchy_of_beat) {
	// NOTE: Only use higher rhythmic levels to check for PARALLELS
	// This doesn't care about whether which voice is the bass. It assumes the composer can add another bass voice
	// Return type is a pair of lists of error and warning messages

	const std::size_t voice_count{ canon.texture().size() };

	// For every voice, generate per-tick note data. MAKES COPIES
	std::vector<std::vector<mx::api::NoteData>> notes_by_tick(voice_count); // Indexed by tick. Ordering of voices doesn't really matter
	for (int i{ 0 }; i < voice_count; ++i) {
		std::vector<mx::api::NoteData> notes_by_tick_for_voice{}; // TODO: predetermine vector size
		for (const mx::api::NoteData& note : canon.texture().at(i)) {
			for (int j{ 0 }; j < note.durationData.durationTimeTicks; ++j) { // Append as many times as the number of ticks the note lasts for
				notes_by_tick_for_voice.emplace_back(note);
			}
		}
		notes_by_tick.at(i) = (notes_by_tick_for_voice);
	}

	// Get every unordered combination of two voices
	std::vector<std::pair<int, int>> voice_pairs{};
	for (int i{ 0 }; i < voice_count; ++i) {
		for (int j{ i + 1 }; j < voice_count; ++j) {
			voice_pairs.emplace_back(std::pair<int, int>{i, j});
		}
	}

	// Start ticks of all dissonances
	std::vector<bool> is_tick_dissonance_start(notes_by_tick.at(0).size());

	// FOR EACH PAIR OF VOICES {
	for (const std::pair<int, int>& voice_pair : voice_pairs) {
		// (different pairs of voices will have different strippings)
			// Generate raw unstripped sonority array
		const Voice& voice_1{ notes_by_tick.at(voice_pair.first) };
		const Voice& voice_2{ notes_by_tick.at(voice_pair.second) };
		SonorityArray raw_sonority_array{};
		for (int i{ 0 }; i < voice_1.size() && i < voice_2.size(); ++i) {
			raw_sonority_array.emplace_back(Sonority{ voice_1.at(i), voice_2.at(i), rhythmic_hierarchy_array.at(i % ticks_per_measure), i });
		}
		// Generate downbeat sonority arrays
		SonorityArray stripped_sonority_array{};
		stripped_sonority_array.emplace_back(raw_sonority_array.at(0)); // Always push the first
		for (int i{ 1 }; i < raw_sonority_array.size(); ++i) {
			if (!is_identical(raw_sonority_array.at(i - 1), raw_sonority_array.at(i))) {
				stripped_sonority_array.emplace_back(raw_sonority_array.at(i));
			}
		}

		for (int i{ 0 }; i < stripped_sonority_array.size() - 1; ++i) {
			stripped_sonority_array.at(i).build_motion_data(stripped_sonority_array.at(i + 1));
		}

		std::vector<std::vector<int>> index_arrays_for_sonority_arrays{};
		for (int depth{ 0 }; depth <= rhythmic_hierarchy_max_depth; ++depth) {
			std::vector<int> index_array_at_depth{};
			for (int i{ 0 }; i < stripped_sonority_array.size(); ++i) {
				if (depth <= stripped_sonority_array.at(i).get_rhythmic_hierarchy()) {
					index_array_at_depth.emplace_back(i);
				}
			}

			index_arrays_for_sonority_arrays.emplace_back(index_array_at_depth);
		}

		// Get the two inversions
		SonorityArray sonority_array_21{ stripped_sonority_array }; // Voice 2 in the bass
		SonorityArray sonority_array_12{ stripped_sonority_array  }; // Voice 1 in the bass
		for (Sonority& sonority : sonority_array_21) {
			sonority.note_1().pitchData.octave += 10;
		}
		for (Sonority& sonority : sonority_array_12) {
			sonority.note_2().pitchData.octave += 10;
		}

		const std::pair<std::vector<int>, std::vector<int>> default_dissonant_intervals{ std::vector<int>{1, 6}, std::vector<int>{} };

		std::vector<Message> sa_21_error_message_box{};
		std::vector<Message> sa_21_warning_message_box{};
		check_with_given_config(default_dissonant_intervals, sa_21_error_message_box, sa_21_warning_message_box, index_arrays_for_sonority_arrays, sonority_array_21, is_tick_dissonance_start, true, ticks_per_measure, tonic, dominant, leading_tone, rhythmic_hierarchy_array, rhythmic_hierarchy_max_depth, rhythmic_hierarchy_of_beat, voice_count);
		const bool sa_21_valid{ sa_21_error_message_box.size() == 0 && sa_21_warning_message_box.size() <= settings::warning_threshold };

		std::vector<Message> sa_12_error_message_box{};
		std::vector<Message> sa_12_warning_message_box{};
		check_with_given_config(default_dissonant_intervals, sa_12_error_message_box, sa_12_warning_message_box, index_arrays_for_sonority_arrays, sonority_array_12, is_tick_dissonance_start, false, ticks_per_measure, tonic, dominant, leading_tone, rhythmic_hierarchy_array, rhythmic_hierarchy_max_depth, rhythmic_hierarchy_of_beat, voice_count);
		// write_to_is_tick_dissonance_start is false this time because whether a note is dissonant doesn't depend on voice order here
		const bool sa_12_valid{ sa_12_error_message_box.size() == 0 && sa_12_warning_message_box.size() <= settings::warning_threshold };

		if (sa_21_valid && sa_12_valid) {
			// canon.error_message_box() is empty by default
			if (sa_21_warning_message_box.size() < sa_12_warning_message_box.size()) {
				canon.warning_message_box() = sa_21_warning_message_box;
			}
			else {
				canon.warning_message_box() = sa_12_warning_message_box;
			}
		}
		else if (sa_21_valid) {
			canon.warning_message_box() = sa_21_warning_message_box;
			canon.add_non_invertible_voice_pair(voice_pair);
		}
		else if (sa_12_valid) {
			canon.warning_message_box() = sa_12_warning_message_box;
			canon.add_non_invertible_voice_pair(std::pair<int, int>{ voice_pair.second, voice_pair.first });
		}
		else {
			// Doesn't matter which. One error disqualifies whole canon
			canon.error_message_box() = sa_21_error_message_box;
		}

	// Check bass/top/outer voice pairs
		bool sa_2b1_valid{ true };
		bool sa_21t_valid{ true };
		if (sa_21_valid) {
		// 2 as bass
			std::vector<Message> sa_2b1_error_message_box{}; // 2-bass, 1
			std::vector<Message> sa_2b1_warning_message_box{};
			check_with_given_config(std::pair<std::vector<int>, std::vector<int>>{ std::vector<int>{1, 3, 6}, std::vector<int>{ 6 } }, sa_2b1_error_message_box, sa_2b1_warning_message_box, index_arrays_for_sonority_arrays, sonority_array_21, is_tick_dissonance_start, false, ticks_per_measure, tonic, dominant, leading_tone, rhythmic_hierarchy_array, rhythmic_hierarchy_max_depth, rhythmic_hierarchy_of_beat, voice_count);
			for (std::vector<int> index_array : index_arrays_for_sonority_arrays) {
				if (index_array.size() > 0) {
					check_outer_voice(sonority_array_21, index_array, 1, sa_2b1_error_message_box, sa_2b1_warning_message_box);
				}
			}
			if (sa_2b1_error_message_box.size() > 0 || sa_2b1_warning_message_box.size() > settings::warning_threshold) {
				canon.add_invalid_bass_voice(1); // Voice 2 is index 1
				sa_2b1_valid = false;
			}

		// 1 as top
			std::vector<Message> sa_21t_error_message_box{}; // 2-bass, 1
			std::vector<Message> sa_21t_warning_message_box{};
			for (std::vector<int> index_array : index_arrays_for_sonority_arrays) {
				if (index_array.size() > 0) {
					check_outer_voice(sonority_array_21, index_array, 0, sa_21t_error_message_box, sa_21t_warning_message_box);
				}
			}
			if (sa_21t_error_message_box.size() > 0 || sa_21t_warning_message_box.size() > settings::warning_threshold) {
				canon.add_invalid_top_voice(0);
				sa_21t_valid = false;
			}
		}
		else {
			sa_2b1_valid = false;
			sa_21t_valid = false;
		}

		bool sa_1b2_valid{ true };
		bool sa_12t_valid{ true };
		if (sa_12_valid) {
			// 1 as bass
			std::vector<Message> sa_1b2_error_message_box{};
			std::vector<Message> sa_1b2_warning_message_box{};
			check_with_given_config(std::pair<std::vector<int>, std::vector<int>>{ std::vector<int>{1, 3, 6}, std::vector<int>{ 6 } }, sa_1b2_error_message_box, sa_1b2_warning_message_box, index_arrays_for_sonority_arrays, sonority_array_12, is_tick_dissonance_start, false, ticks_per_measure, tonic, dominant, leading_tone, rhythmic_hierarchy_array, rhythmic_hierarchy_max_depth, rhythmic_hierarchy_of_beat, voice_count);
			for (std::vector<int> index_array : index_arrays_for_sonority_arrays) {
				if (index_array.size() > 0) {
					check_outer_voice(sonority_array_12, index_array, 0, sa_1b2_error_message_box, sa_1b2_warning_message_box);
				}
			}
			if (sa_1b2_error_message_box.size() > 0 || sa_1b2_warning_message_box.size() > settings::warning_threshold) {
				canon.add_invalid_bass_voice(0);
				sa_1b2_valid = false;
			}

			// 2 as top
			std::vector<Message> sa_12t_error_message_box{};
			std::vector<Message> sa_12t_warning_message_box{};
			for (std::vector<int> index_array : index_arrays_for_sonority_arrays) {
				if (index_array.size() > 0) {
					check_outer_voice(sonority_array_12, index_array, 1, sa_12t_error_message_box, sa_12t_warning_message_box);
				}
			}
			if (sa_12t_error_message_box.size() > 0 || sa_12t_warning_message_box.size() > settings::warning_threshold) {
				canon.add_invalid_top_voice(1);
				sa_12t_valid = false;
			}
		}
		else {
			sa_1b2_valid = false;
			sa_12t_valid = false;
		}
		
	// Both are outer voices
		//if (!sa_2b1_valid || !sa_21t_valid)	{
		//	canon.add_invalid_outer_voice_pair(std::pair<int, int>{ voice_pair.second, voice_pair.first });
		//}
		//else {
			// 2o1o
			std::vector<Message> sa_2o1o_error_message_box{};
			std::vector<Message> sa_2o1o_warning_message_box{};
			for (std::vector<int> index_array : index_arrays_for_sonority_arrays) {
				if (index_array.size() > 0) {
					check_outer_voice_pair(sonority_array_21, index_array, sa_2o1o_error_message_box, sa_2o1o_warning_message_box, voice_count);
				}
			}
			if (sa_2o1o_error_message_box.size() > 0 || sa_2o1o_warning_message_box.size() > settings::warning_threshold) {
				canon.add_invalid_outer_voice_pair(std::pair<int, int>{ voice_pair.second, voice_pair.first });
			}
		//}

		//if (!sa_1b2_valid || !sa_12t_valid) {
		//	canon.add_invalid_outer_voice_pair(voice_pair);
		//}
		//else {
			// 1o2o
			std::vector<Message> sa_1o2o_error_message_box{};
			std::vector<Message> sa_1o2o_warning_message_box{};
			for (std::vector<int> index_array : index_arrays_for_sonority_arrays) {
				if (index_array.size() > 0) {
					check_outer_voice_pair(sonority_array_21, index_array, sa_1o2o_error_message_box, sa_1o2o_warning_message_box, voice_count);
				}
			}

			if (sa_1o2o_error_message_box.size() > 0 || sa_1o2o_warning_message_box.size() > settings::warning_threshold) {
				canon.add_invalid_outer_voice_pair(voice_pair);
			}
		//}

#ifdef DEBUG
			print_messages(canon);
#endif // DEBUG
	}
}