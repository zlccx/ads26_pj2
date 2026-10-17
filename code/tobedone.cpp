#include <ctime>
#include <cstdio>
#include <string>
#include <vector>
#include <algorithm>
#include "dijkstra.h"

// 待处理的状态清理实验，暂不作为报告结论依据
struct ResetAblationRunner {
	ResetAblationRunner(const Graph& g)
		: g_(g), dist(g.n + 1, 0), done(g.n + 1, 0), heap(g.n) {
		settled.reserve(g.n + 1);
	}

	Key run(int s, int t, bool usePartialReset, int& outSettled) {
		if (usePartialReset) {
			// 由上一轮局部清理维持干净状态
		} else {
			fill(done.begin(), done.end(), (char)0);
		}

		heap.clear();
		heap.insert(s, 0);
		dist[s] = 0;
		settled.clear();
		Key ans = KEY_INF;

		while (!heap.empty()) {
			int u = heap.extractMin();
			done[u] = 1;
			settled.push_back(u);

			if (u == t) {
				ans = dist[u];
				break;
			}

			Key du = dist[u];
			for (int i = g_.head[u]; i < g_.head[u + 1]; i ++ ) {
				int v = g_.to[i];
				if (done[v]) {
					continue;
				}
				Key nd = du + g_.w[i];
				if (heap.contains(v)) {
					if (nd < heap.keyOf(v)) {
						heap.decreaseKey(v, nd);
						dist[v] = nd;
					}
				} else {
					dist[v] = nd;
					heap.insert(v, nd);
				}
			}
		}

		outSettled = (int)settled.size();

		if (usePartialReset) {
			for (size_t i = 0; i < settled.size(); i ++ ) {
				done[settled[i]] = 0;
			}
		} else {
			// 全量重置留待下次开局 fill
		}
		return ans;
	}

	const Graph& g_;
	vector<Key> dist;
	vector<char> done;
	vector<int> settled;
	BinaryHeap heap;
};

int main(int argc, char** argv) {
	string graphPath = "data/assets/d/USA-road-d.NY.gr";
	string queryPath = "data/queries/d/NY.in";
	if (argc >= 3) {
		graphPath = argv[1];
		queryPath = argv[2];
	}

	printf("========================================================================\n");
	printf("  Experiment 4: Reset Strategy Ablation (Self-Contained)                \n");
	printf("  Graph: %s\n", graphPath.c_str());
	printf("========================================================================\n");

	Graph g = loadGraph(graphPath);
	vector<int> srcs, dsts;
	if (!loadQueries(queryPath, srcs, dsts)) {
		printf("[Error] Cannot load queries from %s\n", queryPath.c_str());
		return 1;
	}

	ResetAblationRunner runner(g);
	vector<int> smallQueries;
	vector<int> largeQueries;

	for (size_t i = 0; i < srcs.size(); i ++ ) {
		int setCnt = 0;
		runner.run(srcs[i], dsts[i], true, setCnt);
		if (setCnt < 5000) {
			smallQueries.push_back((int)i);
		} else if (setCnt > 50000) {
			largeQueries.push_back((int)i);
		}
	}

	printf("Query Classification: Small Scope = %zu, Large Scope = %zu\n\n",
	       smallQueries.size(), largeQueries.size());

	FILE* fp = fopen("results/reset_results.csv", "w");
	if (fp) {
		fprintf(fp, "scope,full_reset_ms,partial_reset_ms,speedup_ratio\n");
	}

	if (!smallQueries.empty()) {
		clock_t t0 = clock();
		for (int idx : smallQueries) {
			int s = 0;
			runner.run(srcs[idx], dsts[idx], false, s);
		}
		clock_t t1 = clock();
		double fullMs = (1000.0 * (t1 - t0) / CLOCKS_PER_SEC) / smallQueries.size();

		t0 = clock();
		for (int idx : smallQueries) {
			int s = 0;
			runner.run(srcs[idx], dsts[idx], true, s);
		}
		t1 = clock();
		double partialMs = (1000.0 * (t1 - t0) / CLOCKS_PER_SEC) / smallQueries.size();

		double speedup = fullMs / partialMs;
		printf("  Small Search Scope : Full Reset = %7.3f ms | Partial Reset = %7.3f ms | Speedup = %.2fx\n",
		       fullMs, partialMs, speedup);
		if (fp) {
			fprintf(fp, "Small,%.4f,%.4f,%.2f\n", fullMs, partialMs, speedup);
		}
	}

	if (!largeQueries.empty()) {
		clock_t t0 = clock();
		for (int idx : largeQueries) {
			int s = 0;
			runner.run(srcs[idx], dsts[idx], false, s);
		}
		clock_t t1 = clock();
		double fullMs = (1000.0 * (t1 - t0) / CLOCKS_PER_SEC) / largeQueries.size();

		t0 = clock();
		for (int idx : largeQueries) {
			int s = 0;
			runner.run(srcs[idx], dsts[idx], true, s);
		}
		t1 = clock();
		double partialMs = (1000.0 * (t1 - t0) / CLOCKS_PER_SEC) / largeQueries.size();

		double speedup = fullMs / partialMs;
		printf("  Large Search Scope : Full Reset = %7.3f ms | Partial Reset = %7.3f ms | Speedup = %.2fx\n",
		       fullMs, partialMs, speedup);
		if (fp) {
			fprintf(fp, "Large,%.4f,%.4f,%.2f\n", fullMs, partialMs, speedup);
		}
	}

	if (fp) {
		fclose(fp);
		printf("\n[Output] Results written to results/reset_results.csv\n");
	}
	return 0;
}
