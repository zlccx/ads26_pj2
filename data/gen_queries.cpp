#include <cstdio>
#include <random>

// 在项目根目录运行；也可指定一张图和输出文件。
// 每张图生成 1000 对不同起终点，固定种子便于复现实验。
bool generate(const char* graph, const char* output, unsigned seed) {
	FILE* in = fopen(graph, "r");
	if (!in) {
		fprintf(stderr, "Cannot open %s\n", graph);
		return false;
	}
	int n = 0;
	long long m = 0;
	char line[1024];
	while (fgets(line, sizeof(line), in)) {
		if (sscanf(line, "p sp %d %lld", &n, &m) == 2) {
			break;
		}
	}
	fclose(in);
	if (n < 2 || m < 0) {
		fprintf(stderr, "Invalid graph header: %s\n", graph);
		return false;
	}
	FILE* out = fopen(output, "w");
	if (!out) {
		fprintf(stderr, "Cannot open %s\n", output);
		return false;
	}
	std::mt19937 rng(seed);
	std::uniform_int_distribution<int> vertex(1, n);
	fprintf(out, "c 1000 uniform random point-to-point queries, seed=%u\n", seed);
	fprintf(out, "p aux sp p2p 1000\n");
	for (int i = 0; i < 1000; i ++ ) {
		int s = vertex(rng);
		int t = vertex(rng);
		while (s == t) {
			t = vertex(rng);
		}
		fprintf(out, "q %d %d\n", s, t);
	}
	bool ok = !ferror(out);
	if (fclose(out) != 0) {
		ok = false;
	}
	printf("%s -> %s (%s)\n", graph, output, ok ? "OK" : "write failed");
	return ok;
}

int main(int argc, char** argv) {
	if (argc == 3) {
		return generate(argv[1], argv[2], 20261017u) ? 0 : 1;
	}
	if (argc != 1) {
		fprintf(stderr, "Usage: %s [graph.gr output.in]\n", argv[0]);
		return 1;
	}
	const char* nets[] = {"NY", "BAY", "COL", "FLA", "NW", "NE", "CAL",
	                      "LKS", "E", "W", "CTR", "USA"};
	const char kinds[] = {'d', 't'};
	for (char kind : kinds) {
		for (int i = 0; i < 12; i ++ ) {
			char graph[128], output[128];
			snprintf(graph, sizeof(graph), "data/assets/%c/USA-road-%c.%s.gr", kind, kind, nets[i]);
			snprintf(output, sizeof(output), "data/queries/%c/%s.in", kind, nets[i]);
			// 同一路网两个权重版本使用相同种子，便于比较。
			if (!generate(graph, output, 20261017u + i)) {
				return 1;
			}
		}
	}
	return 0;
}
