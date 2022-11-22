/**
 * Framework for Threes! and its variants (C++ 11)
 * agent.h: Define the behavior of variants of agents including players and environments
 *
 * Author: Theory of Computer Games
 *         Computer Games and Intelligence (CGI) Lab, NYCU, Taiwan
 *         https://cgilab.nctu.edu.tw/
 */

#pragma once
#include <string>
#include <random>
#include <sstream>
#include <map>
#include <type_traits>
#include <algorithm>
#include <fstream>
#include <iostream>
#include "board.h"
#include "action.h"
#include "weight.h"

class agent {
public:
	agent(const std::string& args = "") {
		std::stringstream ss("name=unknown role=unknown " + args);
		for (std::string pair; ss >> pair; ) {
			std::string key = pair.substr(0, pair.find('='));
			std::string value = pair.substr(pair.find('=') + 1);
			meta[key] = { value };
		}
	}
	virtual ~agent() {}
	virtual void open_episode(const std::string& flag = "") {}
	virtual void close_episode(const std::string& flag = "") {}
	virtual action take_action(const board& b) { return action(); }
	virtual bool check_for_win(const board& b) { return false; }

public:
	virtual std::string property(const std::string& key) const { return meta.at(key); }
	virtual void notify(const std::string& msg) { meta[msg.substr(0, msg.find('='))] = { msg.substr(msg.find('=') + 1) }; }
	virtual std::string name() const { return property("name"); }
	virtual std::string role() const { return property("role"); }

protected:
	typedef std::string key;
	struct value {
		std::string value;
		operator std::string() const { return value; }
		template<typename numeric, typename = typename std::enable_if<std::is_arithmetic<numeric>::value, numeric>::type>
		operator numeric() const { return numeric(std::stod(value)); }
	};
	std::map<key, value> meta;
};

/**
 * base agent for agents with randomness
 */
class random_agent : public agent {
public:
	random_agent(const std::string& args = "") : agent(args) {
		if (meta.find("seed") != meta.end())
			engine.seed(int(meta["seed"]));
		else
			engine.seed(std::random_device()());
	}
	virtual ~random_agent() {}

protected:
	std::default_random_engine engine;
};

/**
 * base agent for agents with weight tables and a learning rate
 */
class weight_agent : public agent {
public:
	weight_agent(const std::string& args = "") : agent(args), alpha(0.01) {
		if (meta.find("init") != meta.end())
			init_weights(meta["init"]);
		if (meta.find("load") != meta.end())
			load_weights(meta["load"]);
		if (meta.find("alpha") != meta.end())
			alpha = float(meta["alpha"]);
	}
	virtual ~weight_agent() {
		if (meta.find("save") != meta.end())
			save_weights(meta["save"]);
	}

protected:
	virtual void init_weights(const std::string& info) {
		std::string res = info; // comma-separated sizes, e.g., "65536,65536"
		for (char& ch : res)
			if (!std::isdigit(ch)) ch = ' ';
		std::stringstream in(res);
		for (size_t size; in >> size; net.emplace_back(size));
	}
	virtual void load_weights(const std::string& path) {
		// std::cout << "load weight\n";
		std::ifstream in(path, std::ios::in | std::ios::binary);
		if (!in.is_open()) std::exit(-1);
		uint32_t size;
		in.read(reinterpret_cast<char*>(&size), sizeof(size));
		net.resize(size);
		for (weight& w : net) in >> w;
		in.close();
		// std::cout << "load weight check\n";
		// for (auto& wei : net) {
		// 	wei.check();
		// }
	}
	virtual void save_weights(const std::string& path) {
		// std::cout << "save weight\n";
		std::ofstream out(path, std::ios::out | std::ios::binary | std::ios::trunc);
		if (!out.is_open()) std::exit(-1);
		uint32_t size = net.size();
		out.write(reinterpret_cast<char*>(&size), sizeof(size));
		for (weight& w : net) out << w;
		out.close();
		// std::cout << "save weight check\n";
		// for (auto& wei : net) {
		// 	wei.check();
		// }
	}

public:
	weight::type get_potential(const board& brd) const {
		weight::type val = 0;
		for (const auto& wei : net) {
			val += wei.get_weight(brd);
		}
		return val;
	}

	void update(const board& brd, weight::type err) {
		// std::cout << "overall err is " << err << '\n';
		weight::type nerr = err / static_cast<weight::type>(net.size());
		for (auto& wei : net) {
			// if (nerr != 0)
			// 	std::cout << "wei.update " << nerr << '\n';
			wei.update(brd, nerr);
		}
	}

protected:
	std::vector<weight> net;
	float alpha;
};

class n_tuple_slider : public weight_agent {
public:
	n_tuple_slider(const std::vector<weight::pattern>& pats, const std::string& args = "") : weight_agent(args + " name=n_tuple_slider role=player") {
		// std::cout << "n_tuple_slider constructor\n";
		if (net.size()) {
			// std::cout << "only set pattern\n";
			for (unsigned int i = 0; i < net.size(); ++i) {
				net[i].set_pattern(pats[i]);
			}
		}
		else {
			// std::cout << "init new weight\n";
			for (const auto& pat : pats) {
				net.push_back(weight(pat));
			}	
		}
		// std::cout << "n_tuple const check\n";
		// for (auto& wei : net) {
		// 	wei.check();
		// }
	}

	virtual void close_episode(const std::string& flag = "") override {
		if (meta.find("learn") != meta.end() && property("learn") == "no_learn") {
			stats.clear();
			return;
		}
		learn();
	}

	virtual action take_action(const board& before) override {
		// std::cout << "net size " << net.size() << '\n';
		// for (auto& wei : net) {
		// 	wei.check();
		// }
		weight::type best_value = -1e9;
		char best_drct = 5;

		// std::cout << "take action\n";

		stats.push_back({board(), 0, 0});
		for (int i = 0; i < 4; ++i) {
			board after = board(before);
			board::reward rew = after.slide(i);
			if (rew == -1) continue;
			weight::type pot = get_potential(after);
			// std::cout << "drct " << i << " reward " << rew << " pot " << pot << '\n';
			if (best_drct == 5 || pot + rew > best_value) {
				// std::cout << "new best " << pot << ' ' << rew << '\n';
				best_value = pot + rew, best_drct = i;
				stats.back() = {after, pot, rew};
			}
		}
		// std::cout << "best si drct " << best_drct << " with value " << best_value << '\n';

		// no valid move
		if (best_drct == 5) {
			return action();
		}
		// std::cout << "best is " << (int)best_drct << '\n';
		return action::slide(best_drct);
	}

	void learn() {
		// if (stats[stats.size() - 2].after.max() <= 3) {
		// 	std::cout << "bad??\n";
		// 	for (auto& st : stats) {
		// 		st.after.show();
		// 		std::cout << '\n';
		// 	}
		// }
		weight::type new_pot = 0;
		for (stats.pop_back(); stats.size(); stats.pop_back()) {
			
			auto& cur = stats.back();
			// std::cout << get_potential(cur.after) << '\t';
			// if (new_pot != 0)
			// std::cout << new_pot << '\t';
			// std::cout << "the diff is " << new_pot - cur.pot << '\n';
			// if (new_pot - cur.pot > 0) {
			// 	std::cout << " + \t";
			// }
			// else {
			// 	std::cout << " - \t";
			// }
			update(cur.after, alpha * (new_pot - cur.pot));
			new_pot = cur.rew + get_potential(cur.after);
			// std::cout << get_potential(cur.after) << '\n';
		}
	}

protected:
	struct stat {
		board after;
		weight::type pot;
		board::reward rew;
	};

	std::vector<stat> stats;
};

class four_tuple_slider : public n_tuple_slider {
public:
	four_tuple_slider(const std::string& args = "") : n_tuple_slider({
		{0, 1, 2, 3},
		{4, 5, 6, 7}
	}, args + " name=four_tuple_slider") {}
};

class six_tuple_slider : public n_tuple_slider {
public:
	six_tuple_slider(const std::string& args = "") : n_tuple_slider({
		{0, 1, 2, 3, 4, 5},
		{4, 5, 6, 7, 8, 9},
		{0, 1, 2, 4, 5, 6},
		{4, 5, 6, 8, 9, 10}
	}, args + " name=six_tuple_slider") {}
};

class best_six_tuple_slider : public n_tuple_slider {
public:
	best_six_tuple_slider(const std::string& args = "") : n_tuple_slider({
		{0, 1, 2, 3, 4, 5},
		{4, 5, 6, 7, 8, 9},
		{0, 1, 2, 4, 5, 6},
		{0, 1, 5, 6, 7, 10},
		{0, 1, 2, 5, 9, 10},
		{0, 1, 5, 9, 13, 14}
	}, args + " name=best_six_tuple_slider") {}
};

class best_eight_tuple_slider : public n_tuple_slider {
public:
	best_eight_tuple_slider(const std::string& args = "") : n_tuple_slider({
		{0, 1, 2, 3, 4, 5},
		{4, 5, 6, 7, 8, 9},
		{0, 1, 2, 4, 5, 6},
		{0, 1, 5, 6, 7, 10},
		{0, 1, 2, 5, 9, 10},
		{0, 1, 5, 9, 13, 14},
		{0, 1, 5, 8, 9, 13},
		{0, 1, 2, 4, 6, 10}
	}, args + " name=best_eight_tuple_slider") {}
};

/**
 * default random environment, i.e., placer
 * place the hint tile and decide a new hint tile
 */
class random_placer : public random_agent {
public:
	random_placer(const std::string& args = "") : random_agent("name=place role=placer " + args) {
		spaces[0] = { 12, 13, 14, 15 };
		spaces[1] = { 0, 4, 8, 12 };
		spaces[2] = { 0, 1, 2, 3};
		spaces[3] = { 3, 7, 11, 15 };
		spaces[4] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
	}

	virtual action take_action(const board& after) {
		std::vector<int> space = spaces[after.last()];
		std::shuffle(space.begin(), space.end(), engine);
		for (int pos : space) {
			if (after(pos) != 0) continue;

			int bag[3], num = 0;
			for (board::cell t = 1; t <= 3; t++)
				for (size_t i = 0; i < after.bag(t); i++)
					bag[num++] = t;
			std::shuffle(bag, bag + num, engine);

			board::cell tile = after.hint() ?: bag[--num];
			board::cell hint = bag[--num];

			return action::place(pos, tile, hint);
		}
		return action();
	}

private:
	std::vector<int> spaces[5];
};

/**
 * random player, i.e., slider
 * select a legal action randomly
 */
class random_slider : public random_agent {
public:
	random_slider(const std::string& args = "") : random_agent("name=slide role=slider " + args),
		opcode({ 0, 1, 2, 3 }) {}

	virtual action take_action(const board& before) {
		std::shuffle(opcode.begin(), opcode.end(), engine);
		for (int op : opcode) {
			board::reward reward = board(before).slide(op);
			if (reward != -1) return action::slide(op);
		}
		return action();
	}

private:
	std::array<int, 4> opcode;
};

class left_down_alternaing_slider : public agent {
public:
	left_down_alternaing_slider(const std::string& args = "") : agent("name=slide role=slider " + args) {}

	virtual action take_action(const board& before) {
		go_left = !go_left;
		if (go_left && can(before, left)) return action::slide(left);
		if (can(before, down)) return action::slide(down);
		if (can(before, up)) return action::slide(up);
		if (can(before, right)) return action::slide(right);
		return action();
	}

	bool can(const board& before, unsigned op) {
		return board(before).slide(op) != -1;
	}

private:
	bool go_left = 0;
	const unsigned up = 0, right = 1, down = 2, left = 3;

};

class left_down_priority_slider : public agent {
public:
	left_down_priority_slider(const std::string& args = "") : agent("name=slide role=slider " + args) {}

	virtual action take_action(const board& before) {
		for (int i = 0; i < 4; ++i) {
			unsigned op = pq[prv][i];
			if (can(before, op)) {
				prv = op;
				return action::slide(op);
			}
		}
		return action();
	}

	bool can(const board& before, unsigned op) {
		return board(before).slide(op) != -1;
	}

private:
	unsigned prv = 0;
	const unsigned up = 0, right = 1, down = 2, left = 3;

	const unsigned pq[4][4] = {
		{down, left, up, right}, // up
		{left, down, right, up}, // right
		{left, down, up, right}, // down
		{down, left, right, up} // left
	};
};

class all_perm_slider : public agent {
public:
	all_perm_slider(const std::string& args = "") : agent("name=slide role=slider " + args) {
		int rule = meta.count("rule")? meta["rule"] : 213774;
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < rule % 24; ++j) {
				std::next_permutation(pq[i], pq[i] + 4);
			}
			rule /= 24;
		}
	}

	virtual action take_action(const board& before) {
		for (int i = 0; i < 4; ++i) {
			unsigned op = pq[prv][i];
			if (can(before, op)) {
				prv = op;
				return action::slide(op);
			}
		}
		return action();
	}

	bool can(const board& before, unsigned op) {
		return board(before).slide(op) != -1;
	}

private:
	unsigned prv = 0;
	const unsigned up = 0, right = 1, down = 2, left = 3;
	unsigned pq[4][4] = {
		{0, 1, 2, 3},
		{0, 1, 2, 3},
		{0, 1, 2, 3},
		{0, 1, 2, 3}
	};
};