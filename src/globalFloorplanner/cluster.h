#include "globmodule.h"
#include <vector>
#include <tuple>
#include <map>
#include <set>
#include <algorithm>


class Cluster {
private:
    double m;
    std::set<GlobalModule *> modules;
    std::map<std::pair<GlobalModule *, GlobalModule *>, double> edges;
    std::map<GlobalModule *, double> k;
    std::map<GlobalModule *, int> communities;
    std::map<GlobalModule *, std::set<GlobalModule *>> connectedModules;
public:
    Cluster(std::vector<ConnectionInfo *> connectionList);
    ~Cluster();
    double calculateModularity();
    void louvain();
    std::vector<std::vector<GlobalModule *>> getCluster();
};
