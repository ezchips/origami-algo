#include <bits/stdc++.h>
using namespace std;
#define int int64_t

int n, m; // # of nodes and edges
vector<vector<int>> neighbors; // stores weight of each edge (0 if DNE)
vector<int> numNeighbors; // number of neighbors for each node
vector<bool> visited; // used in dfs, keeps track of nodes alr visited
bool cyclic; // stores whether or not the graph is cyclic
vector<vector<int>> cycles; // stores the nodes in a cyclic path
vector<int> nodes; // node types
				   //leaf: 0, internal: 1
vector<vector<int>> features; // feature type
									   // flap: 0, river: 1, DNE: 2
vector<vector<vector<vector<pair<char,int>>>>> polygons; // stores the polygons under each node
vector<int> flapSize; // stores flap sizes of flap polygons
vector<int> riverSize; // stores river sizes of river polygons
int middle; // center of the graph
int maxTotal = 0; // distance of central node to other nodes
int total; // sum distance of one node to every other node
int pos; // number of polygons
vector<vector<int>> grid; // final matrix of packed polygons
map<int, char> polygonType; // shows the feature corresponding to each polygon
vector<pair<int,int>> temp; // used to sort the polygons based on vertical height
int s; // side length of the square
map<int,int> featuresToPath; // maps each feature to the path it is located on
map<int, bool> rotated; // stores which paths have been rotated
vector<vector<vector<vector<int>>>> creases; // stores creases between points
											 // 0 - no crease
											 // 1 - hinge
											 // 2 - ridge
											 // 3 - axial

// function that checks every node in a graph (used to check how many cycles there are)
void dfs (int node, int i, int start, vector<int> path) {
	path.push_back(node);
	visited[node] = true;
	for (int x = 0; x < neighbors[node].size(); x++) {
		if (neighbors[node][x] == 0) {
			continue;
		}
		if (!visited[x]) {
			dfs(x, i+1, start, path);
		} else {
			if (i > 1 && x == start) {
				cyclic = true;
				cycles.push_back(path);
			}
		}
	}
}

// finds total distance of a node to every other node
void sumDistance (int node, int curDist) {
	total += curDist;
	visited[node] = true;
	for (int i = 0; i < n; i++) {
		if (neighbors[node][i] > 0 && ! visited[i]) {
			sumDistance(i, curDist+1);
		}
	}
}

// creates a flap
void addFlap(int node, int weight) {
	vector<vector<pair<char,int>>> flap (2*weight, vector<pair<char,int>> (2*weight));
	for (int i = 0; i < 2 * weight; i++) {
		for (int j = 0; j < 2 * weight; j++) {
			flap[i][j] = {'F',pos};
		}
	}

	polygons[node].push_back(vector<vector<pair<char,int>>> (flap.size()));
	for (int i = 0; i < flap.size(); i++) {
		for (int j = 0; j < flap.size(); j++) {
			polygons[node][polygons[node].size()-1][i].push_back(flap[i][j]);
		}
	}
	flapSize[pos] = weight;
	pos++;
}

// creates a river
void addRiver(int node1, int node2) {
	vector<vector<pair<char,int>>> river;

	int weight = neighbors[node1][node2];
	for (int i = 0; i < weight; i++) {
		river.push_back(vector<pair<char,int>> ());
	}

	int len = 0;
	for (int i = 0; i < polygons[node2].size(); i++) {
		for (int j = 0; j < polygons[node2][i].size(); j++) {
			int len2 = polygons[node2][i][j].size();
			len = max(len, len2);
			river.push_back(vector<pair<char,int>> ());
			for (int k = 0; k < weight; k++) {
				river[river.size()-1].push_back({'R', pos});
			}
			for (int k = 0; k < len2; k++) {
				river[river.size()-1].push_back(polygons[node2][i][j][k]);
			}
			for (int k = 0; k < weight; k++) {
				river[river.size()-1].push_back({'R', pos});
			}
		}
	}

	len += 2 * neighbors[node1][node2];

	for (int i = 0; i < weight; i++) {
		river.push_back(vector<pair<char,int>> ());
		for (int j = 0; j < len; j++) {
			river[i].push_back({'R', pos});
			river[river.size()-1].push_back({'R', pos});
		}
	}

	for (int i = 0; i < river.size(); i++) {
		if (river[i].size() < len) {
			int numR = 0;
			for (int j = 0; j < river[i].size(); j++) {
				if (river[i][j].first == 'R') {
					numR++;
				}
			}

			int id;

			for (int j = river[i].size()-1; j >= 0; j--) {
				if (river[i][j].first == 'F') {
					id = j;
					break;
				}
			}

			for (int j = id+1; j < len; j++) {
				if (j < river[i].size()) {
					river[i][j] = river[i][j-1];
				} else {
					river[i].push_back(river[i][j-1]);
				}
			}

			for (int j = 0; j < numR/2; j++) {
				river[i][len - 1 - j] = {'R', pos};
			}
		}
	}

	polygons[node1].push_back(vector<vector<pair<char,int>>> (river.size()));
	for (int i = 0; i < river.size(); i++) {
		for (int j = 0; j < len; j++) {
			polygons[node1][polygons[node1].size()-1][i].push_back(river[i][j]);
		}
	}
	riverSize[pos] = weight;
	pos++;
}

// function to create the polygons for each feature
void makePolygons (int node) {
	visited[node] = true;
	for (int i = 0; i < n; i++) {
		if (neighbors[node][i] > 0 && !visited[i]) {
			if (features[node][i] == 0) {
				addFlap(node, neighbors[node][i]);
			} else {
				makePolygons(i);
				addRiver(node, i);
			}
		}
	}
}

bool canPack (int i) {
	int h = polygons[middle][i].size();
	int w = polygons[middle][i][0].size();

	// checks vertically
	for (int x = 0; x < s - h + 1; x++) {
		for (int y = 0; y < s - w + 1; y++) {
			if (grid[x][y] == -1) {
				bool valid = true;
				for (int x2 = 0; x2 < h; x2++) {
					for (int y2 = 0; y2 < w; y2++) {
						if (grid[x+x2][y+y2] != -1) {
							valid = false;
							break;
						}
					}
					if (!valid) {
						break;
					}
				}
				if (valid) {
					return true;
				}
			}
		}
	}

	// checks horizontally
	for (int x = 0; x < s - w + 1; x++) {
		for (int y = 0; y < s - h + 1; y++) {
			if (grid[x][y] == -1) {
				bool valid = true;
				for (int x2 = 0; x2 < w; x2++) {
					for (int y2 = 0; y2 < h; y2++) {
						if (grid[x+x2][y+y2] != -1) {
							valid = false;
							break;
						}
					}
					if (!valid) {
						break;
					}
				}
				if (valid) {
					rotated[i] = true;
					return true;
				}
			}
		}
	}
	return false;
}

// packs the next polygon
void packPolygon (int i) {
	int h = polygons[middle][i].size();
	int w = polygons[middle][i][0].size();

	if (rotated[i]) {
		for (int x = 0; x < s - w + 1; x++) {
			for (int y = 0; y < s - h + 1; y++) {
				if (grid[x][y] == -1) {
					bool valid = true;
					for (int x2 = 0; x2 < w; x2++) {
						for (int y2 = 0; y2 < h; y2++) {
							if (grid[x+x2][y+y2] != -1) {
								valid = false;
								break;
							}
						}
						if (!valid) {
							break;
						}
					}
					if (valid) {
						for (int x2 = 0; x2 < w; x2++) {
							for (int y2 = 0; y2 < h; y2++) {
								grid[x+x2][y+y2] = i;
							}
						}
						return;
					}
				}
			}
		}
	} else {
		for (int x = 0; x < s - h + 1; x++) {
			for (int y = 0; y < s - w + 1; y++) {
				if (grid[x][y] == -1) {
					bool valid = true;
					for (int x2 = 0; x2 < h; x2++) {
						for (int y2 = 0; y2 < w; y2++) {
							if (grid[x+x2][y+y2] != -1) {
								valid = false;
								break;
							}
						}
						if (!valid) {
							break;
						}
					}
					if (valid) {
						for (int x2 = 0; x2 < h; x2++) {
							for (int y2 = 0; y2 < w; y2++) {
								grid[x+x2][y+y2] = i;
							}
						}
						return;
					}
				}
			}
		}
	}
}

// expands the polygon
void expandPolygon (int i, int amt, int dir) {
	int h = polygons[middle][i].size();
	int w = polygons[middle][i][0].size();
	if (rotated[i]) {
		dir = 1 - dir;
	}

	if (dir == 0) { // expand to the right
		int copy = w;
		for (int x = 0; x < h; x++) {
			w = copy;
			vector<int> half;
			int id = w/2;
			for (int y = w - 1; y >= w/2; y--) {
				half.push_back(polygons[middle][i][x][y].second);
				if (polygons[middle][i][x][y].first == 'F') {
					id = y;
					break;
				}
			}

			for (int y = 0; y < amt; y++) {
				polygons[middle][i][x].push_back({'L',-1});
				w++;
			}
			
			for (int y = 0; y < id + amt; y++) {
				polygons[middle][i][x][w-1-y].second = half[half.size()-1];
			}

			for (int y = 0; y < half.size(); y++) {
				polygons[middle][i][x][w-1-y].second = half[y];
			}
			w = copy;
		}
	} else { // expand to the bottom
		int row;
		bool found = false;
		for (int x = h - 1; x >= 0; x--) {
			for (int y = 0; y < w; y++) {
				if (polygons[middle][i][x][y].first == 'F') {
					found = true;
					row = x;
					break;
				}
			}
			if (found) {
				break;
			}
		}

		vector<vector<pair<char,int>>> half;
		for (int x = row; x < h; x++) {
			half.push_back(vector<pair<char,int>> ());
			for (int y = 0; y < w; y++) {
				half[half.size()-1].push_back(polygons[middle][i][x][y]);
			}
		}

		for (int x = 0; x < amt; x++) {
			polygons[middle][i].push_back(vector<pair<char,int>> (w));
			h++;
		}

		for (int x = row; x < h; x++) {
			for (int y = 0; y < w; y++) {
				polygons[middle][i][x][y] = half[0][y];
			}
		}

		for (int x = 0; x < half.size(); x++) {
			for (int y = 0; y < w; y++) {
				polygons[middle][i][h-1-x][y] = half[half.size()-1-x][y];
			}
		}
	}
}

// fills in the gaps at each step
void fillGaps (int i) {
	bool found = false;
	int minx, miny;
	for (int j = 0; j < s; j++) {
		for (int k = 0; k < s; k++) {
			if (grid[j][k] == i) {
				minx = k;
				miny = j;
				found = true;
				break;
			}
		}
		if (found) {
			break;
		}
	}

	int h = polygons[middle][i].size();
	int w = polygons[middle][i][0].size();

	if (rotated[i]) {
		int tempp = h;
		h = w;
		w = tempp;
	}

	int maxx = minx + w - 1;
	int maxy = miny + h - 1;
	for (int r = 0; r < s; r++) {
		for (int c = 0; c < s; c++) {
			if (grid[r][c] == -1) {
				if (r >= miny && r <= maxy && c < minx) {
					int path = grid[r-1][c];
					int amt = 0;
					int hh = polygons[middle][path].size();
					int ww = polygons[middle][path][0].size();
					if (rotated[path]) {
						int tempp = hh;
						hh = ww;
						ww = tempp;
					}
					for (int j = r; j <= maxy; j++) {
						amt++;
						for (int k = c; k < c + ww; k++) {
							grid[j][k] = path;
						}
					}
					if (amt > 0) {
						expandPolygon(path, amt, 1);
					}
				} else if (r < miny && c >= minx && c <= maxx) {
					int path = grid[r][c-1];
					int amt = 0;
					int hh = polygons[middle][path].size();
					int ww = polygons[middle][path][0].size();
					if (rotated[path]) {
						int tempp = hh;
						hh = ww;
						ww = tempp;
					}
					for (int k = c; k <= maxx; k++) {
						amt++;
						for (int j = r; j < r + hh; j++) {
							grid[j][k] = path;
						}
					}
					if (amt > 0) {
						expandPolygon(path, amt, 0);
					}
				} else if (r >= miny && r <= maxy && c > maxx) {
					int above = grid[miny-1][maxx];
					int rightMost = c-1;
					for (int k = maxx+1; k < s; k++) {
						if (grid[miny-1][k] == above) {
							rightMost = k;
						}
					}

					int amt = 0;
					for (int k = c; k <= rightMost; k++) {
						amt++;
						for (int j = miny; j <= maxy; j++) {
							grid[j][k] = i;
						}
					}

					if (amt > 0) {
						expandPolygon(i, amt, 0);
						maxx = rightMost;
					}
				} else if (r > maxy && c >= minx && c <= maxx) {
					int left = grid[maxy][minx-1];
					int downMost = r-1;
					for (int j = maxy+1; j < s; j++) {
						if (grid[j][minx-1] == left) {
							downMost = j;
						}
					}

					int amt = 0;
					for (int j = r; j <= downMost; j++) {
						amt++;
						for (int k = minx; k <= maxx; k++) {
							grid[j][k] = i;
						}
					}

					if (amt > 0) {
						expandPolygon(i, amt, 1);
						maxy = downMost;
					}
				}
			}
		}
	}
}

// fills in gaps at the very end
void fillSquare(int i) {
	int minx = s, miny = s, maxx = 0, maxy = 0;
	
	for (int j = 0; j < s; j++) {
		for (int k = 0; k < s; k++) {
			if (grid[j][k] == i) {
				minx = min(minx, k);
				maxx = max(maxx, k);
				miny = min(miny, j);
				maxy = max(maxy, j);
			}
		}
	}
	
	map<int,bool> markedX;
	map<int,bool> markedY;

	for (int j = miny; j < s; j++) {
		for (int k = minx; k < s; k++) {
			if (grid[j][k] == i) continue;
			if (grid[j][k-1] == i && grid[j][k] == -1) {
				grid[j][k] = i;
				if (!markedX[k]) {
					expandPolygon(i, 1, 0);
				}
				markedX[k] = true;
				maxx = k;
			}
			if (grid[j-1][k] == i && grid[j][k] == -1) {
				grid[j][k] = i;
				if (!markedY[j]) {
					expandPolygon(i, 1, 1);
				}
				markedY[j] = true;
				maxy = j;
			}
		}
	}

	for (int r = 0; r < s; r++) {
		for (int c = 0; c < s; c++) {
			if (grid[r][c] == -1) {
				if (r >= miny && r <= maxy && c < minx) {
					int path = grid[r-1][c];
					int amt = s - r;
					int hh = polygons[middle][path].size();
					int ww = polygons[middle][path][0].size();
					if (rotated[path]) {
						int tempp = hh;
						hh = ww;
						ww = tempp;
					}
					for (int r2 = r; r2 < s; r2++) {
						for (int c2 = c; c2 < maxx; c2++) {
							grid[r2][c2] = path;
						}
					}
					expandPolygon(path, amt, 1);
				} else if (r < miny && c >= minx && c <= maxx) {
					int path = grid[r][c-1];
					int amt = s - c;
					int hh = polygons[middle][path].size();
					int ww = polygons[middle][path][0].size();
					if (rotated[path]) {
						int tempp = hh;
						hh = ww;
						ww = tempp;
					}
					for (int r2 = r; r2 < maxy; r2++) {
						for (int c2 = c; c2 < s; c2++) {
							grid[r2][c2] = path;
						}
					}
					expandPolygon(path, amt, 0);
				} else if (r >= miny && r <= maxy && c > maxx) {
					// int amt = s - c;
					// for (int r2 = miny; r2 <= maxy; r2++) {
					// 	for (int c2 = c; c2 < s; c2++) {
					// 		grid[r2][c2] = i;
					// 	}
					// }
					// expandPolygon(i, amt, 0);
				} else if (r > maxy && c >= minx && c <= maxx) {
					// int amt = s - r;
					// for (int r2 = r; r2 < s; r2++) {
					// 	for (int c2 = minx; c2 <= maxx; c2++) {
					// 		grid[r2][c2] = i;
					// 	}
					// }
					// expandPolygon(i, amt, 1);
				}
			}
		}
	}
}

void printCreases(int creaseType) {
	cout << "\n";
	for (int i = 0; i <= s; i++) {
		for (int j = 0; j <= s; j++) {
			cout << ".";
			if (j == s) {
				cout << "\n";
				break;
			}

			if (creases[j][i][j+1][i] == creaseType) {
				cout << "---";
			} else {
				cout << "   ";
			}
		}
		if (i != s) {
			for (int j = 0; j <= s; j++) {
				if (creases[j][i][j][i+1] == creaseType) {
					cout << "| ";
				} else {
					cout << "  ";
				}

				if (j != s) {
					if (creases[j][i][j+1][i+1] == creaseType || creases[j+1][i+1][j][i] == creaseType) {
						cout << "\\ ";
					} else if (creases[j][i+1][j+1][i] == creaseType || creases[j+1][i][j][i+1] == creaseType) {
						cout << "/ ";
					} else {
						cout << "  ";
					}
				}
			}
			cout << "\n";
		}
	}
}

int32_t main() {
	// reads in the input
    cin >> n >> m;
	neighbors.resize(1000, vector<int> (1000, 0));
	numNeighbors.resize(1000, 0);
	visited.resize(1000, false);
	for (int i = 0; i < m; i++) {
		int a, b, c;
		cin >> a >> b >> c;
		neighbors[a][b] = c;
		neighbors[b][a] = c;
		numNeighbors[a]++;
		numNeighbors[b]++;
	}

	// checks for a cycle at each node
	cyclic = false;
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			visited[j] = false;
		}
		dfs(i, 0, i, vector<int> ());
	}

	// if cyclic, determines number of cycles and breaks them
	if (cyclic) {
		// sorts each cycle
		for (int i = 0; i < cycles.size(); i++) {
			sort(cycles[i].begin(), cycles[i].end());
		}

		// marks the indices that are repeats of prior ones
		map<int,bool> repeated;
		for (int i = 0; i < cycles.size(); i++) {
			for (int j = i + 1; j < cycles.size(); j++) {
				if (repeated[j]) continue;
				repeated[j] = true;
				for (int k = 0; k < cycles[i].size(); k++) {
					if (cycles[i][k] != cycles[j][k]) {
						repeated[j] = false;
						break;
					}
				}
			}
		}

		// counts number of unique cycles and counts num of occurences for each node
		map<int,int> count;
		for (int i = 0; i < cycles.size(); i++) {
			if (repeated[i]) continue;

			for (int j = 0; j < cycles[i].size(); j++) {
				count[cycles[i][j]]++;
			}
		}
		
		// breaks cyclic graph
		for (int i = 0; i < cycles.size(); i++) {
			// picks nodes to split
			if (repeated[i]) continue;
			int cur = numNeighbors[cycles[i][0]];
			int index = cycles[i][0];
			map<int,bool> present;
			present[index] = true;
			for (int j = 1; j < cycles[i].size(); j++) {
				int node = cycles[i][j];
				present[node] = true;
				if (count[node] == 1 && numNeighbors[node] < cur) {
					index = node;
					cur = numNeighbors[node];
				}
			}

			// finds two adjacent nodes in the cyclic path
			int cn1 = -1, cn2;
			for (int j = 0; j < n; j++) {
				if (neighbors[index][j] > 0 && present[j]) {
					if (cn1 == -1) {
						cn1 = j;
					} else {
						cn2 = j;
						break;
					}
					
				}
			}

			// removes and adds nodes
			if (numNeighbors[index] > 2) {
				n += 2;

				// adds new node
				neighbors[n-2][cn1] = neighbors[index][cn1] + 1;
				neighbors[cn1][n-2] = neighbors[index][cn1] + 1;

				// removes edge
				neighbors[index][cn1] = 0;
				neighbors[cn1][index] = 0;

				// adds another new node
				neighbors[n-1][index] = 1;
				neighbors[index][n-1] = 1;
			} else {
				n++;

				// extends edge
				neighbors[index][cn1]++;
				neighbors[cn1][index]++;

				// adds new node
				neighbors[n-1][cn2] = neighbors[index][cn2] + 1;
				neighbors[cn2][n-1] = neighbors[index][cn2] + 1;

				// removes edge
				neighbors[index][cn2] = 0;
				neighbors[cn2][index] = 0;
			}
		}
	}

	nodes.resize(n);
	features.resize(n, vector<int> (n));

	// recounts number of neighbors for each node
	for (int i = 0; i < n; i++) {
		numNeighbors[i] = 0;
		for (int j = 0; j < n; j++) {
			if (neighbors[i][j] > 0) {
				numNeighbors[i]++;
			}
		}
	}

	// determines the type of every node
	for (int i = 0; i < n; i++) {
		if (numNeighbors[i] == 1) { // one edge --> leaf node
			nodes[i] = 0;
		} else { // more than one edge --> internal node
			nodes[i] = 1;
		}
	}

	// determines what every edge represents (flap or river) and the size
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			if (neighbors[i][j] == 0) { // checks if edge exists
				features[i][j] = 2;
				features[j][i] = 2;
			} else {
				if (nodes[i] == 0 || nodes[j] == 0) { // checks if edge is connected to a leaf node
					features[i][j] = 0;
					features[j][i] = 0;
				} else { // chceks if edge connects two internal nodes
					features[i][j] = 1;
					features[j][i] = 1;
				}
			}
		}
	}

	maxTotal = 1e9;
	middle = 0;

	// finding the "middle" of the graph
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			visited[j] = false;
		}
		total = 0;
		sumDistance(i, 0);
		if (total < maxTotal) {
			maxTotal = total;
			middle = i;
		}
	}

	for (int i = 0; i < n; i++) {
		visited[i] = false;
	}

	polygons.resize(n, vector<vector<vector<pair<char, int>>>> ());
	flapSize.resize(1000);
	riverSize.resize(1000);
	pos = 0;

	// designs polygons representing each path starting from the middle node
	makePolygons(middle);

	// sorts the polygons based on vertical height
	for (int i = 0; i < polygons[middle].size(); i++) {
		temp.push_back({polygons[middle][i].size(), i});
	}
	sort(temp.rbegin(), temp.rend());

	for (int i = 0; i < polygons[middle].size(); i++) {
		for (int j = 0; j < polygons[middle][i].size(); j++) {
			for (int k = 0; k < polygons[middle][i][j].size(); k++) {
				featuresToPath[polygons[middle][i][j][k].second] = i;
				if (polygons[middle][i][j][k].first == 'F') {
					polygonType[polygons[middle][i][j][k].second] = 'F';
				} else {
					polygonType[polygons[middle][i][j][k].second] = 'R';
				}
			}
		}
	}

	// shows what features each polygon represents
	for (int i = 0; i < pos; i++) {
		cout << i << ": " << polygonType[i] << " " << featuresToPath[i];
		if (polygonType[i] == 'F') {
			cout << " " << flapSize[i];
		} else {
			cout << " " << riverSize[i];
		}
		cout << "\n";
	}

	for (int tmp = 0; tmp < temp.size(); tmp++) {
		int i = temp[tmp].second;
		if (tmp == 0) {
			int h = polygons[middle][i].size();
			int w = polygons[middle][i][0].size();
			s = h;
			for (int j = 0; j < h; j++) {
				if (j >= grid.size()) {
					grid.push_back(vector<int> ());
				}
				for (int k = 0; k < s; k++) {
					if (k < w) {
						grid[j].push_back(i);
					} else {
						grid[j].push_back(-1);
					}
					
				}
			}
		} else {
			while (true) {
				if (canPack(i)) {
					break;
				}
				s++;
				grid.push_back(vector<int> ());
				for (int j = 0; j < grid.size(); j++) {
					while (grid[j].size() < grid.size()) {
						grid[j].push_back(-1);
					}
				}
			}
			packPolygon(i);
			fillGaps(i);
		}
		if (tmp == temp.size() - 1) {
			fillSquare(i);
		}
	}

	for (int i = polygons[middle].size() - 1; i >= 0; i--) {
		bool found = false;
		for (int r = 0; r < s; r++) {
			for (int c = 0; c < s; c++) {
				if (grid[r][c] == i) {
					int h = polygons[middle][i].size();
					int w = polygons[middle][i][0].size();
					if (!rotated[i]) {
						for (int r2 = 0; r2 < h; r2++) {
							for (int c2 = 0; c2 < w; c2++) {
								grid[r2+r][c2+c] = polygons[middle][i][r2][c2].second;
							}
						}
					} else {
						for (int r2 = 0; r2 < h; r2++) {
							for (int c2 = 0; c2 < w; c2++) {
								grid[c2+r][r2+c] = polygons[middle][i][r2][c2].second;
							}
						}
					}
					found = true;
					break;
				}
			}
			if (found) {
				break;
			}
		}
	}

	// prints final polyogn packing
	cout << "\n";
	for (int j = 0; j < s; j++) {
		for (int k = 0; k < s; k++) {
			cout << grid[j][k] << " ";
			if (grid[j][k] < 10) {
				cout << " ";
			}
		}
		cout << "\n";
	}
	cout << "\n";

	vector<vector<int>> newGrid;
	int x1 = 1e9, y1 = 1e9, x2 = 0, y2 = 0;

	for (int i = 0; i <= s; i++) {
		creases.push_back(vector<vector<vector<int>>> ());
		for (int j = 0; j <= s; j++) {
			creases[i].push_back(vector<vector<int>> ());
			for (int k = 0; k <= s; k++) {
				creases[i][j].push_back(vector<int> ());
				for (int l = 0; l <= s; l++) {
					creases[i][j][k].push_back(0);
				}
			}
		}
	}
	
	for (int x = 0; x < pos; x++) {
		int minix = s, miniy = s, maxix = 0, maxiy = 0;
		for (int j = 0; j < s; j++) {
			for (int k = 0; k < s; k++) {
				if (grid[j][k] == x) {
					minix = min(minix, k);
					maxix = max(maxix, k);
					miniy = min(miniy, j);
					maxiy = max(maxiy, j);
				}
			}
		}

		// adding hinge creases
		for (int i = minix; i <= maxix; i++) {
			creases[i][miniy][i+1][miniy] = 1;
			creases[i][maxiy+1][i+1][maxiy+1] = 1;
		}

		for (int j = miniy; j <= maxiy; j++) {
			creases[minix][j][minix][j+1] = 1;
			creases[maxix+1][j][maxix+1][j+1] = 1;
		}

		if (polygonType[x] == 'R') {
			int w = riverSize[x];
			for (int i = minix+w; i <= maxix-w; i++) {
				creases[i][miniy+w][i+1][miniy+w] = 1;
				creases[i][maxiy+1-w][i+1][maxiy+1-w] = 1;
			}

			for (int j = miniy+w; j <= maxiy-w; j++) {
				creases[minix+w][j][minix+w][j+1] = 1;
				creases[maxix+1-w][j][maxix+1-w][j+1] = 1;
			}
		}


		// ridge creases
		if (polygonType[x] == 'F') {
			if (maxix - minix == maxiy - miniy) {
				for (int i = minix; i <= maxix; i++) {
					creases[i][miniy+i-minix][i+1][miniy+1+i-minix] = 2;
					creases[i][maxiy+1-i+minix][i+1][maxiy-i+minix] = 2;
				}
			} else if (maxix - minix > maxiy - miniy) {
				int amt = (miniy + maxiy + 1) / 2;
				amt = amt - miniy;
				for (int i = 0; i < amt; i++) {
					creases[minix+i][miniy+i][minix+1+i][miniy+1+i] = 2;
					creases[maxix+1-i][miniy+i][maxix-i][miniy+1+i] = 2;
					creases[minix+i][maxiy+1-i][minix+1+i][maxiy-i] = 2;
					creases[maxix+1-i][maxiy+1-i][maxix-i][maxiy-i] = 2;
				}

				for (int i = minix + amt; i < maxix - amt + 1; i++) {
					creases[i][miniy+amt][i+1][miniy+amt] = 2;
				}
			} else {
				int amt = (minix + maxix + 1) / 2;
				amt = amt - minix;
				for (int i = 0; i < amt; i++) {
					creases[minix+i][miniy+i][minix+1+i][miniy+1+i] = 2;
					creases[maxix+1-i][miniy+i][maxix-i][miniy+1+i] = 2;
					creases[minix+i][maxiy+1-i][minix+1+i][maxiy-i] = 2;
					creases[maxix+1-i][maxiy+1-i][maxix-i][maxiy-i] = 2;
				}

				for (int i = miniy + amt; i < maxiy - amt + 1; i++) {
					creases[minix+amt][i][minix+amt][i+1] = 2;
				}
			}
		} else {
			int w = riverSize[x];
			for (int i = 0; i < w; i++) {
				creases[minix+i][miniy+i][minix+i+1][miniy+i+1] = 2;
				creases[minix+i][maxiy+1-i][minix+i+1][maxiy-i] = 2;
			}

			for (int i = 0; i < w; i++) {
				creases[maxix+1-i][miniy+i][maxix-i][miniy+i+1] = 2;
				creases[maxix+1-i][maxiy+1-i][maxix-i][maxiy-i] = 2;
			}
		}
	}

	map<pair<int,int>, bool> stop;
	for (int i = 0; i <= s; i++) {
		for (int j = 0; j <= s; j++) {
			if (i != s) {
				if (creases[i][j][i+1][j] != 0 || creases[i+1][j][i][j] != 0) {
					stop[{i,j}] = true;
					stop[{i+1,j}] = true;
				}
			} 

			if (j != s) {
				if (creases[i][j][i][j+1] != 0 || creases[i][j+1][i][j] != 0) {
					stop[{i,j}] = true;
					stop[{i,j+1}] = true;
				}
			}

			if (i != s && j != s) {
				if (i != s && j != s && creases[i][j][i+1][j+1] != 0 || creases[i+1][j+1][i][j] != 0) {
					stop[{i,j}] = true;
					stop[{i+1,j+1}] = true;
				}
			}
			
			if (i != 0 && j != s) {
				if (i != 0 && j != s && creases[i][j][i-1][j+1] != 0 || creases[i-1][j+1][i][j] != 0) {
					stop[{i,j}] = true;
					stop[{i-1,j+1}] = true;
				}
			}
		}
	}

	for (int x = 0; x < pos; x++) {
		int minix = s, miniy = s, maxix = 0, maxiy = 0;
		for (int j = 0; j < s; j++) {
			for (int k = 0; k < s; k++) {
				if (grid[j][k] == x) {
					minix = min(minix, k);
					maxix = max(maxix, k);
					miniy = min(miniy, j);
					maxiy = max(maxiy, j);
				}
			}
		}

		for (int j = minix; j <= maxix+1; j++) {
			for (int k = miniy; k <= maxiy+1; k++) {
				if (creases[j][k][j][k+1] == 0) {
					creases[j][k][j][k+1] = 3;
				} else {
					break;
				}

				if (stop[{j,k+1}]) {
					break;
				}
			}
		}

		for (int j = minix; j <= maxix+1; j++) {
			for (int k = maxiy+1; k >= miniy; k--) {
				if (creases[j][k-1][j][k] == 0) {
					creases[j][k-1][j][k] = 3;
				} else {
					break;
				}

				if (stop[{j,k-1}]) {
					break;
				}
			}
		}

		for (int k = miniy; k <= maxiy+1; k++) {
			for (int j = minix; j <= maxix+1; j++) {
				if (creases[j][k][j+1][k] == 0) {
					creases[j][k][j+1][k] = 3;
				} else {
					break;
				}

				if (stop[{j+1,k}]) {
					break;
				}
			}	
		}

		for (int k = miniy; k <= maxiy+1; k++) {
			for (int j = maxix+1; j >= minix; j--) {
				if (creases[j-1][k][j][k] == 0) {
					creases[j-1][k][j][k] = 3;
				} else {
					break;
				}

				if (stop[{j-1,k}]) {
					break;
				}
			}	
		}
	}

	
	cout << "Hinge Creases";
	printCreases(1);
	cout << "\nRidge Creases";
	printCreases(2);
	cout << "\nAxial Creases";
	printCreases(3);
	cout << "\nStructural Crease Pattern\n";
	for (int i = 0; i <= s; i++) {
		for (int j = 0; j <= s; j++) {
			cout << ".";
			if (j == s) {
				cout << "\n";
				break;
			}

			if (creases[j][i][j+1][i] != 0) {
				cout << "---";
			} else {
				cout << "   ";
			}
		}
		if (i != s) {
			for (int j = 0; j <= s; j++) {
				if (creases[j][i][j][i+1] != 0) {
					cout << "| ";
				} else {
					cout << "  ";
				}

				if (j != s) {
					if (creases[j][i][j+1][i+1] != 0 || creases[j+1][i+1][j][i] != 0) {
						cout << "\\ ";
					} else if (creases[j][i+1][j+1][i] != 0 || creases[j+1][i][j][i+1] != 0) {
						cout << "/ ";
					} else {
						cout << "  ";
					}
				}
			}
			cout << "\n";
		}
	}
}