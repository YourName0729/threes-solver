#pragma once

#include <map>
#include "agent.h"

// remember to set proper alpha, lambda, and depth
// max_depth should be even, so terminal node is a max node
class td_lambda : public weight_agent {
public:
	td_lambda(const std::string& args = "") : weight_agent(args + " name=n_tuple_slider role=player"), lambda(0.5), max_depth(7), v_init(0) {
        if (meta.find("pats") == meta.end()) meta["pats"] = agent::value{"best_eight"};
		if (meta.find("v_init") != meta.end())
			v_init = weight::type(meta["v_init"]);
		if (net.size()) {
			for (unsigned int i = 0; i < net.size(); ++i) {
				net[i].set_pattern(pats[meta["pats"]][i]);
			}
		}
		else {
			for (const auto& pat : pats[meta["pats"]]) {
				net.push_back(weight(pat, v_init));
			}	
		}
        if (meta.find("lambda") != meta.end())
			lambda = float(meta["lambda"]);
		if (meta.find("max_depth") != meta.end())
			max_depth = int(meta["max_depth"]);
	}

	virtual void close_episode(const std::string& flag = "") override {
		if (meta.find("learn") != meta.end() && property("learn") == "no_learn") {
			stats.clear();
			return;
		}
		// std::cout << '\n';
		learn();
	}

	int hit = 0, miss = 0;
	virtual action take_action(const board& before) override {
		// std::cout << "#";
		stats.push_back({board(), 0, 0});
		trans_tab.clear();
		weight::type bst = -1e9;
		int bst_drct = -1;
		hit = miss = 0;
		for (int i = 0; i < 4; ++i) {
			board after = before;
			auto rew = after.slide(i);
			if (rew == -1) continue;
			weight::type score = rew + dfs(after, 0, false);
			if (bst < score) {
				bst = score, bst_drct = i;
				stats.back() = {after, get_potential(after), rew};
			}
		}
		if (bst_drct == -1) {
			
			return action();
		}
		return action::slide(bst_drct);
	}

protected:
	
	weight::type dfs(const board& state, int dep, bool is_max) {
		// is_max == true: slider's turn -> max
		// is_max == false: placer's turn -> expect
		if (trans_tab.count(state)) return trans_tab[state];
		if (dep == max_depth) return trans_tab[state] = get_potential(state);
		if (is_max) {
			// slider's turn
			weight::type bst = -1e9;
			for (unsigned int i = 0; i < 4; ++i) {
				board after = state;
				auto rew = after.slide(i);
				if (rew == -1) continue;
				bst = std::max(bst, rew + dfs(after, dep + 1, false));
			}
			if (bst == -1e9) return 0;
			return trans_tab[state] = bst;
		}
		else {
			// placer's turn
			static std::vector<int> spaces[5] = {
				{ 12, 13, 14, 15 },
				{ 0, 4, 8, 12 },
				{ 0, 1, 2, 3},
				{ 3, 7, 11, 15 },
				{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 }
			};
			weight::type sum = 0, n = 0;
			for (auto pos : spaces[state.last()]) {
				if (state(pos) != 0) continue;

				int bag[3], num = 0;
				for (board::cell t = 1; t <= 3; t++)
					for (size_t i = 0; i < state.bag(t); i++)
						bag[num++] = t;
				auto upd = [&](board::cell tile, board::cell hint) {
					board after = state;
					auto rew = after.place(pos, tile, hint);
					// if (rew == -1) std::cout << "rew?????\n";
					++n;
					sum += rew + dfs(after, dep + 1, true);	
				};

				if (state.hint()) {
					for (int i = 0; i < num; ++i) {
						upd(state.hint(), bag[i]);
					}
				}
				else {
					for (int i = 0; i < num; ++i) {
						for (int j = 0; j < num; ++j) {
							if (i == j) continue;
							upd(bag[i], bag[j]);
						}
					}
				}
			}
			// if (n == 0) std::cout << "n????\n";
			return trans_tab[state] = sum / n;
		}
		return 0;
	}


	void learn() {
		// std::cout << "learn\n";
		weight::type target = 0;
		for (stats.pop_back(); stats.size(); stats.pop_back()) {
			auto& cur = stats.back();
			update(cur.after, alpha * (target - cur.pot));
			target = cur.rew + (1 - lambda) * get_potential(cur.after) + lambda * target;
		}
		// std::cout << "end learn\n";
	}

protected:
	struct stat {
		board after;
		weight::type pot; // potential of the after
		board::reward rew; // reward got from a state to "after"
	};

    float lambda;
	int max_depth;
	weight::type v_init;
	std::vector<stat> stats; // for leanring
	
	// static std::array<std::array<std::size_t, 20>, 16> zob; // Zobrist hashing
	struct hasher {
		std::size_t operator()(const board& b) const {
			const std::size_t zob[16][20] = {
				{0xe7fd4ce02d9298d8, 0x0cb9bfa03ac092f3, 0x0b8c5783a759e401, 0x6cd8b2c0da1e26ae, 0x07ca1895aaffe55a, 0x7bf05f06d41dd465, 0xfcd9637cd4c38ea0, 0x3478667d474091e5, 0x3afef71586242c86, 0x0dcbf192d9925d4b, 0x830c2166969b6d17, 0xcb271613b99bbd01, 0xc390e7df2e8d7c8c, 0xb9630f6c4789c636, 0x38d77de7b3b8daaa, 0xc3c475c19f0f67ba, 0x257af6d3c1da3d60, 0x214b81a8c32d59a7, 0x8b368e14287ba81e, 0xebf1825cc988c863},
				{0x69a710be87de072e, 0xf7aac7a87b8cc062, 0x2671a4dbc436a4ad, 0x57c4caef12bc56b5, 0x89e4f85807472a6c, 0x7b8e1961ee12434e, 0x68b66b3a1d9b606a, 0xba0c1ee4fb52e938, 0x9e2dcf3baa4c0923, 0x49fd2d5971653adc, 0xc6657a6ec6ff4b6e, 0xc8c3b2b6ad086aac, 0x55b3d7df4d75239c, 0x1d71dce7c6f4f2ea, 0x72ecceb6663ce261, 0x02b50b9ced46b529, 0x5e8aff4674d04573, 0xb05186ab2f7d3079, 0x67ebe5ff40e3ab8c, 0xdf178558060c4a68},
				{0x36f32329cd4763cd, 0xdcfbe79cc859bd8b, 0xde3e168b837f5b46, 0x1303304a20a2288a, 0xc5c529938f47da7c, 0x1d85e2688b8f8d49, 0x3a551482e617b5ff, 0xb25697aa5ca6dae7, 0x7dce2b4a5dc16c52, 0x44696c6238f0c718, 0xc15d4d23a374cd97, 0x41fc8fb3508b6d05, 0x6be288629b478ae0, 0x18612f76c761f80e, 0x871de5bedb373ab3, 0xf8f08ac38c8df87e, 0x72ed5085a8cf5537, 0x45538a2749d931fa, 0x3c54cb6bd49bd266, 0xd50ed6a32e2320cb},
				{0x153d86bf9fbd494a, 0x7afcd871efdaad59, 0xa9f75ea536f28639, 0x5185356bbad90f6f, 0x090b14d848ded95c, 0x47c93fb6bbc81e7b, 0x03886bd8549ef9b5, 0xf187c32eca234278, 0x433618199bf94f68, 0xcf7e6d772c92eeb8, 0xb426af26ded25c40, 0x5257e56547cc9ae6, 0xf84da89a03bd5b58, 0xfcc225f969917c05, 0x3a5ed889e839f50f, 0xcb5ad6a14419ce9d, 0xf2896a25ed0debe6, 0x562bd07854594b51, 0x33e4093230a0389f, 0xaaf4c5ae2bebead7},
				{0xb1ba10f09676ba82, 0x3f2eb9faacadf7a9, 0x5672ac362b470a86, 0xe09f7fb70c794870, 0x3a7f5ff116eb0f9c, 0x5cfc3b9b736b336c, 0xfdb5755e31871f31, 0xa71c0a13a7459cbc, 0xc29dab377033fb02, 0xe1a3fa7ba29753c8, 0xcd03c857b2a01292, 0x291402481cf42e3e, 0xe4b7efa4bcd02fca, 0x744886aab9e22df3, 0xb9da7cc607042f0f, 0x16e00e28c4036bf7, 0x55c9f6f325c119d5, 0x5414857a4e3da1c0, 0x472bd2c3ff868841, 0x2713b0acdf1a02bd},
				{0x53f2ac7c726aee31, 0xf2752ce18bda2884, 0xafd31066d46a3450, 0x7a48682fe640738c, 0x55e18797bb9558a0, 0x5eb11db101658fe3, 0x6698281584804831, 0xa89772793b5dbc18, 0xa00a89fda7ab4d20, 0x6da80bf330b285f6, 0x8a4bef0bd5485538, 0xca002cee7b0e6821, 0xa30c33215fb35ef8, 0xd25a545107453aab, 0x4a923ac7e25aa8e6, 0xb4323b0f4ac0c701, 0xc6791d55590855a3, 0x2daffc78c311567d, 0x652eb1a2a4673080, 0xcd56a77732c1b082},
				{0x5bc6ece352ba48ff, 0xa0306486917093e1, 0x549df04cc7a03aa6, 0x7fbea7407cb4256b, 0x326818505688efc5, 0xaab2fce66f29ca69, 0xf042a0d78b948d5f, 0xc01493a88d853db8, 0x96d34f9c3ad47efd, 0x2ff4e81f2f93aa72, 0x73f97c0f309b8a2f, 0xb655cb961bfd6393, 0x08610b1f94be9fd9, 0xc7f0bc7ffc0f5a93, 0x33df9e4c6ca5a01e, 0xd781e825c55bfa87, 0xeeae5d4b38281fa3, 0x9163f4ce2d2c8e38, 0x65d5606d20075734, 0x7b5051673ee14526},
				{0x0dbb6b42e0109a1a, 0x17900b2805590b61, 0xfc5a036d01c9997d, 0x8824fa1746ac66d6, 0x5f3266be4f2696cf, 0x3a4c56d7ea826188, 0x963b52ecc897a526, 0x5f7275bda957ab6c, 0x03819e98579fe52e, 0x2117a442a9e427f6, 0x44a94ee47c510893, 0xbf2dc150ad2925b4, 0xafe5a39c5b1d225a, 0x103bab9c3a10c164, 0x0305299883c75454, 0x65fa43560ace29b4, 0x77b41405404e5d1e, 0x21452ee705006932, 0xce7cf3c120dd9d14, 0x5c9ee9e897a33df4},
				{0xc53d2c33d1507c30, 0xabfcdad89b16c9af, 0x11f59ace5f36bf43, 0xd4be46ef5d507a70, 0x767de7c72e01d7f4, 0x1363a81519d1aa9b, 0xea6cfceb88f1612b, 0x11bd76601457ab98, 0xb1853b637de9d88a, 0x0af80d7c9a9ae863, 0x81f8328193992eca, 0xcc9bd4f4e308502f, 0xfc7ffab5fa4d6fbe, 0x8fc452e9852fd301, 0x192e11e8da7bc3d4, 0xf94ec44bdf3b73d4, 0xa24e7d9414f5bc74, 0xd3365b685e804ed8, 0x195f6b51a9805761, 0x9922e0161cc2b2a0},
				{0xa36d54c02ffb7452, 0x50390cb461ff628f, 0x17f9cc1fc1a9bb7e, 0x553d1e246a18c214, 0x265ee8aa287ee9cb, 0x5a015af1b5ef82df, 0xbcb0348035cfbd61, 0x5deff469f1704b97, 0xd8cd82bfd5537a5f, 0x98b471a1fa512701, 0x6e007cb30bf55dd1, 0x5b105e9cb3271463, 0x38eafc1cb2f07510, 0x399e6ab8b86842e0, 0x3d7cedff33e74cb9, 0xd86df824d5c8182f, 0x61829f33073aca01, 0x939d4a2a286d5118, 0x143abc696764c965, 0x459483669c2cbd18},
				{0x57745469ced404f1, 0x659cdd43a493307b, 0x40740dc404988f5d, 0xa4de3c1ae101a575, 0x7726354afe528b23, 0x2fa1623059e5e7ac, 0xb32560a169435821, 0x6549b81d8088012a, 0x0e9b3564252feb89, 0x4f50a5d9322f53ad, 0x5647076cc9564b9a, 0x0cbdf4b86323758d, 0xc7507d1e4d4de19a, 0x31ecf6be30c3436d, 0xefe7d922fd257555, 0x42a7eda0b3251cdc, 0x9588b50c2e2c200a, 0xf9f555b54bca797e, 0x9c1afe15efd52006, 0xdfc9458eaec9be5f},
				{0x523f7e94c4223108, 0xbcf666c2b5d24c62, 0xe0f3c730c5691e65, 0x0a0fc67afb209c86, 0xda9cfda987328039, 0x19ef60657d862a17, 0x8d2ffdff5edb86b8, 0x8b2bec4eb4a0d47c, 0x601403838396f94e, 0xb0b7e590afa40fb9, 0x6cd0629a96b34804, 0x865646df7b373071, 0x088914569d086ee8, 0x0680c081c3ba3240, 0x66526b6bc0ccc0c5, 0x3f17f7400073fc83, 0xa493c4fbb0865933, 0xa89d23d026c28444, 0x9d399de96e788ecf, 0x89474ab2a9d74773},
				{0x1375119c07da40b3, 0xbba44c7dd0a56415, 0x3062addb7b7e48e0, 0x6da476b5da4b2b51, 0x50e8c69d80ed5b9d, 0x89d3781a8d98a013, 0x2e8b59f7c3525b14, 0xf861072c72bba945, 0x5b7fe0e5e30e18ef, 0xc57cb9b2e9176027, 0x472067c9247b2aa7, 0x180fbd4b826ab4a7, 0x30ad29acae620696, 0x1a971ea0045b172c, 0xfb7fcaf317cf18ff, 0x41ba159d38dae065, 0x439608c8cfde6f60, 0xc3e883fd1cc3f792, 0xfa3ebfa672396c73, 0xc902302ab94b0a74},
				{0x751156f5368483ca, 0xa5de564db871523a, 0xbd0d31a9663ce69a, 0xff6f5b745200e552, 0xfc129122dda6b1d0, 0x17af65239974c0b4, 0x4a42741af8a0b392, 0xb12f4eeeed13e105, 0x0fbbec18b6231a5e, 0x4aae722bdadad7eb, 0xd9e5b0da85e85ea7, 0xaaf288b8061469b9, 0x1b036b808f003200, 0xbfc120524160613a, 0x6b28089df5fd0dd4, 0x8a4e8085a677d07f, 0x65575784fd0a0ba3, 0x8447a935e84f9d30, 0x82234d4caec4cea4, 0x23d6bfc840376fde},
				{0x3da71aa1d4ef5b5d, 0xa1a5e4499dc84cbe, 0x388d78603d7eeab3, 0x053a4d8ed88a9db3, 0xefb40cc4ec9847f4, 0xf2176652f423d773, 0xfec9a81ed7564d71, 0x18846769d7317d70, 0xed5f333654bd76df, 0x94eaae6d231a96cc, 0x3b12a6cd1ec01fcb, 0xbbb5acc9b9f21e1f, 0x02317f70f685abea, 0x9904381363ec98a2, 0xf652bc330a3c4e8a, 0x991a85f55439c20d, 0x322d7335c318a660, 0x742ab4c1eed9457a, 0x4e689233ad0bf468, 0x885d52602c23472a},
				{0xe69fdbabf7a2426a, 0xef41d02e5691c07e, 0x50a11b673f993bb8, 0xeafe338b81df0c00, 0xaa55fd53bd7fb08b, 0xd35bf41c26824d61, 0x74435df0948cf884, 0xf7d25cc04849217b, 0x34710ed2034477ca, 0x4a40606ae7f3f2b0, 0xf760ccfe0ae50b4d, 0x1338362d51f94704, 0x187491f138611f0b, 0x7356d342c53618eb, 0x0860e84002dc2cbe, 0x32a068b7b7f5246a, 0x92d5050a732a36b0, 0xe1eb2776a32ec59f, 0x633454fc749f747d, 0x2db5faad55e1f0d6}
			};

			std::size_t re = 0;
			for (int i = 0; i < 16; ++i) {
				re ^= zob[i][b(i)];
			}
			return re;
		}
	};
	std::unordered_map<board, weight::type, hasher> trans_tab;

    static inline std::map<std::string, std::vector<weight::pattern>> pats = {
        {"best_eight", {
			{0, 1, 2, 3, 4, 5},
			{4, 5, 6, 7, 8, 9},
			{0, 1, 2, 4, 5, 6},
			{0, 1, 5, 6, 7, 10},
			{0, 1, 2, 5, 9, 10},
			{0, 1, 5, 9, 13, 14},
			{0, 1, 5, 8, 9, 13},
			{0, 1, 2, 4, 6, 10}
		}}
    };
};