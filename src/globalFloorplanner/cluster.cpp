#include "cluster.h"

Cluster::Cluster(std::vector<ConnectionInfo *> connectionList) {
    for ( ConnectionInfo *conn : connectionList ) {
        std::vector<GlobalModule *> modPtrs = conn->modulePtrs;
        std::sort(modPtrs.begin(), modPtrs.end());
        double weight = ( double ) conn->value / ( modPtrs.size() - 1 );
        std::pair< GlobalModule *, GlobalModule *> modPair;
        for ( int i = 0; i < modPtrs.size() - 1; ++i ) {
            this->modules.emplace(modPtrs[i]);
            for ( int j = i + 1; j < modPtrs.size(); ++j ) {
                modPair = std::make_pair(modPtrs[i], modPtrs[j]);
                if ( this->edges.count(modPair) != 0 ) {
                    this->edges[modPair] += weight;
                }
                else {
                    this->edges[modPair] = weight;
                }
            }
        }
        this->modules.emplace(modPtrs.back());
    }

    int c = 0;
    for ( GlobalModule *mod : this->modules ) {
        this->k[mod] = 0;
        this->communities[mod] = c++;
    }

    for ( const std::pair<std::pair<GlobalModule *, GlobalModule *>, double> &edge : this->edges ) {
        GlobalModule *mod1 = edge.first.first;
        GlobalModule *mod2 = edge.first.second;
        double weight = edge.second;
        this->m += weight;
        this->k[mod1] += weight;
        this->k[mod2] += weight;
        this->connectedModules[mod1].emplace(mod2);
        this->connectedModules[mod2].emplace(mod1);
    }
}

Cluster::~Cluster() { }

double Cluster::calculateModularity() {
    std::map<int, double> weightInCommunities;

    for ( const std::pair<std::pair<GlobalModule *, GlobalModule *>, double> &edge : this->edges ) {
        GlobalModule *mod1 = edge.first.first;
        GlobalModule *mod2 = edge.first.second;
        double weight = edge.second;
        if ( this->communities[mod1] == this->communities[mod2] ) {
            weightInCommunities[this->communities[mod1]] += weight;
        }
    }

    double modularity = 0;
    for ( const std::pair<int, double> &inner : weightInCommunities ) {
        int community = inner.first;
        double sum_in = inner.second;
        double sum_tot = 0;
        for ( GlobalModule *mod : this->modules ) {
            if ( this->communities[mod] == community ) {
                sum_tot += this->k[mod];
            }
        }
        modularity += ( sum_in / ( 2 * this->m ) ) - ( sum_tot / ( 2 * this->m ) ) * ( sum_tot / ( 2 * this->m ) );
    }
    return modularity;
}

void Cluster::louvain() {
    for ( GlobalModule *curMod : this->modules ) {
        double curModularity = this->calculateModularity();
        double bestModularity = -2.;
        int bestCommunity;
        int curCommunity = this->communities[curMod];
        for ( GlobalModule *adjMod : this->connectedModules[curMod] ) {
            if ( this->communities[adjMod] == curCommunity ) {
                continue;
            }
            // setting current module to different community
            int tarCommunity = this->communities[adjMod];
            this->communities[curMod] = tarCommunity;
            double modularityAfter = this->calculateModularity();
            if ( modularityAfter > bestModularity ) {
                bestCommunity = tarCommunity;
                bestModularity = modularityAfter;
            }
        }
        if ( bestModularity > curModularity ) {
            this->communities[curMod] = bestCommunity;
        }
        else {
            this->communities[curMod] = curCommunity;
        }
    }
}

std::vector<std::vector<GlobalModule *>> Cluster::getCluster() {
    std::map<int, std::vector<GlobalModule *>> clusterMap;
    for ( const std::pair<GlobalModule *, int> &community : this->communities ) {
        clusterMap[community.second].push_back(community.first);
    }

    std::vector<std::vector<GlobalModule *>> cluster;
    for ( const std::pair<int, std::vector<GlobalModule *>> &group : clusterMap ) {
        cluster.push_back(group.second);
    }
    return cluster;
}