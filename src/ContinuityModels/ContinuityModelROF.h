//
// Created by bernardo on 1/26/17.
//

#ifndef TRIANGLEMODEL_CONTINUITYMODELROF_H
#define TRIANGLEMODEL_CONTINUITYMODELROF_H


#include "Base/ContinuityModel.h"
#include "../Utils/Matrices.h"

class ContinuityModelROF : public ContinuityModel {
private:
    Matrix3D<double> *storage_to_rof_table;
    Matrix3D<double> storage_to_rof_realization;
    bool *storage_wout_downstream;
    int downstreammost_source = NON_INITIALIZED;

protected:
    int beginning_tier = 0;
    vector<WaterSource *> realization_water_sources;

public:
    ContinuityModelROF(
            vector<WaterSource *> water_sources,
            const Graph &water_sources_graph,
            const vector<vector<int>> &water_sources_to_utilities,
            vector<Utility *> utilities,
            vector<MinEnvironFlowControl *> &min_env_flow_controls,
            const unsigned int realization_id);

    ContinuityModelROF(ContinuityModelROF &continuity_model_rof);

    vector<double> calculateShortTermROF(int week);

    vector<double> calculateLongTermROF(int week);

    void resetUtilitiesAndReservoirs(int rof_type);

    void setRealization_water_sources(const vector<WaterSource *> &water_sources_realization);

    void updateOnlineInfrastructure(int week);

    virtual ~ContinuityModelROF();

    void updateStorageToROFTable(double storage_percent_decrement, int week_of_the_year,
                                     const double *to_full);

    void shiftStorages(double *storages, const double *deltas);

    const Matrix3D<double> *getStorage_to_rof_table() const;
};


#endif //TRIANGLEMODEL_CONTINUITYMODELROF_H
