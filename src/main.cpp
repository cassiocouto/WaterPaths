#include <iostream>
#include <algorithm>
#include "SystemComponents/WaterSources/Reservoir.h"
#include "SystemComponents/Utility.h"
#include "Utils/Utils.h"
#include "Simulation/Simulation.h"
#include "Utils/QPSolver/QuadProg++.h"
#include "DroughtMitigationInstruments/Transfers.h"
#include "SystemComponents/WaterSources/ReservoirExpansion.h"
#include "SystemComponents/WaterSources/Quarry.h"
#include "SystemComponents/WaterSources/WaterReuse.h"
#include "Controls/SeasonalMinEnvFlowControl.h"
#include "Controls/StorageMinEnvFlowControl.h"
#include "Controls/InflowMinEnvFlowControl.h"
#include "Controls/FixedMinEnvFlowControl.h"
#include "Controls/Custom/JordanLakeMinEnvFlowControl.h"
#include "Controls/Custom/FallsLakeMinEnvFlowControl.h"
#include "SystemComponents/WaterSources/AllocatedReservoir.h"
#include "SystemComponents/WaterSources/SequentialJointTreatmentExpansion.h"
#include "SystemComponents/WaterSources/Relocation.h"
#include "DroughtMitigationInstruments/InsuranceStorageToROF.h"
#include "Utils/Solutions.h"



using namespace std;
using namespace Constants;
using namespace Solutions;

struct infraRank {
    int id;
    double xreal;

    infraRank(int id, double xreal) {
        this->id = id;
        this->xreal = xreal;
    }
};

struct by_xreal {
    bool operator()(const infraRank &ir1, const infraRank &ir2) {
        return ir1.xreal < ir2.xreal;
    }
};

vector<int> vecInfraRankToVecInt(vector<infraRank> v) {
    vector<int> sorted;
    for (infraRank ir : v) {
        sorted.push_back(ir.id);
    }

    return sorted;
}

double checkAndFixInfraExpansionHighLowOrder(
        vector<int> *order, int id_low,
        int id_high, double capacity_low, double capacity_high) {

    long pos_low = distance(order->begin(),
                            find(order->begin(),
                                 order->end(),
                                 id_low));

    long pos_high = distance(order->begin(),
                             find(order->begin(),
                                  order->end(),
                                  id_high));

    if (pos_high < pos_low) {
        capacity_high += capacity_low;
        order->erase(order->begin() + pos_low);
    }

    return capacity_high;
}


void triangleTest(int n_threads, const double *x_real, int n_realizations,
                  int n_weeks, int sol_number, string output_directory) {
    srand((unsigned int) time(nullptr));

    // ===================== SET UP DECISION VARIABLES  =====================

    double Durham_restriction_trigger = x_real[0];
    double OWASA_restriction_trigger = x_real[1];
    double raleigh_restriction_trigger = x_real[2];
    double cary_restriction_trigger = x_real[3];
    double durham_transfer_trigger = x_real[4];
    double owasa_transfer_trigger = x_real[5];
    double raleigh_transfer_trigger = x_real[6];
    double OWASA_JLA = x_real[7];
    double Raleigh_JLA = x_real[8];
    double Durham_JLA = x_real[9];
    double Cary_JLA = x_real[10];
    double durham_annual_payment = x_real[11]; // contingency fund
    double owasa_annual_payment = x_real[12];
    double raleigh_annual_payment = x_real[13];
    double cary_annual_payment = x_real[14];
    double durham_insurance_use = x_real[15]; // insurance st_rof
    double owasa_insurance_use = x_real[16];
    double raleigh_insurance_use = x_real[17];
    double cary_insurance_use = x_real[18];
    double durham_insurance_payment = x_real[19];
    double owasa_insurance_payment = x_real[20];
    double raleigh_insurance_payment = x_real[21];
    double cary_insurance_payment = x_real[22];
    double durham_inftrigger = x_real[23];
    double owasa_inftrigger = x_real[24];
    double raleigh_inftrigger = x_real[25];
    double cary_inftrigger = x_real[26];
    double university_lake_expansion_ranking = x_real[27]; // 14
    double Cane_creek_expansion_ranking = x_real[28]; // 24
    double Stone_quarry_reservoir_expansion_shallow_ranking = x_real[29]; // 12
    double Stone_quarry_reservoir_expansion_deep_ranking = x_real[30]; // 13
    double Teer_quarry_expansion_ranking = x_real[31]; // 9
    double reclaimed_water_ranking_low = x_real[32]; // 18
    double reclaimed_water_high = x_real[33]; // 19
    double lake_michie_expansion_ranking_low = x_real[34]; // 15
    double lake_michie_expansion_ranking_high = x_real[35]; // 16
    double little_river_reservoir_ranking = x_real[36]; // 7
    double richland_creek_quarry_rank = x_real[37]; // 8
    double neuse_river_intake_rank = x_real[38]; // 10
    double reallocate_falls_lake_rank = x_real[39]; // 17
    double western_wake_treatment_plant_rank_OWASA_low = x_real[40]; // 20
    double western_wake_treatment_plant_rank_OWASA_high = x_real[41]; // 21
    double western_wake_treatment_plant_rank_durham_low = x_real[42]; // 20
    double western_wake_treatment_plant_rank_durham_high = x_real[43]; // 21
    double western_wake_treatment_plant_rank_raleigh_low = x_real[44]; // 20
    double western_wake_treatment_plant_rank_raleigh_high = x_real[45]; // 21
//    double caryupgrades_1 = x_real[46]; // not used: already built.
    double caryupgrades_2 = x_real[47];
    double caryupgrades_3 = x_real[48];
    double western_wake_treatment_plant_owasa_frac = x_real[49];
    double western_wake_treatment_frac_durham = x_real[50];
    double western_wake_treatment_plant_raleigh_frac = x_real[51];
    double falls_lake_reallocation = x_real[52];
    double durham_inf_buffer = x_real[53];
    double owasa_inf_buffer = x_real[54];
    double raleigh_inf_buffer = x_real[55];
    double cary_inf_buffer = x_real[56];

    vector<infraRank> durham_infra_order_raw = {
            infraRank(9,
                      Teer_quarry_expansion_ranking),
            infraRank(15,
                      lake_michie_expansion_ranking_low),
            infraRank(16,
                      lake_michie_expansion_ranking_high),
            infraRank(18,
                      reclaimed_water_ranking_low),
            infraRank(19,
                      reclaimed_water_high),
            infraRank(20,
                      western_wake_treatment_plant_rank_durham_low),
            infraRank(21,
                      western_wake_treatment_plant_rank_durham_high)
    };

    vector<infraRank> owasa_infra_order_raw = {
            infraRank(12,
                      Stone_quarry_reservoir_expansion_shallow_ranking),
            infraRank(13,
                      Stone_quarry_reservoir_expansion_deep_ranking),
            infraRank(14,
                      university_lake_expansion_ranking),
            infraRank(24,
                      Cane_creek_expansion_ranking),
            infraRank(20,
                      western_wake_treatment_plant_rank_OWASA_low),
            infraRank(21,
                      western_wake_treatment_plant_rank_OWASA_high)
    };

    vector<infraRank> raleigh_infra_order_raw = {
            infraRank(7,
                      little_river_reservoir_ranking),
            infraRank(8,
                      richland_creek_quarry_rank),
            infraRank(10,
                      neuse_river_intake_rank),
            infraRank(17,
                      reallocate_falls_lake_rank),
            infraRank(20,
                      western_wake_treatment_plant_rank_raleigh_low),
            infraRank(21,
                      western_wake_treatment_plant_rank_raleigh_high)
    };

    double added_storage_michie_expansion_low = 2500;
    double added_storage_michie_expansion_high = 5200;
    double reclaimed_capacity_low = 2.2 * 7;
    double reclaimed_capacity_high = 9.1 * 7;

    sort(durham_infra_order_raw.begin(),
         durham_infra_order_raw.end(),
         by_xreal());
    sort(owasa_infra_order_raw.begin(),
         owasa_infra_order_raw.end(),
         by_xreal());
    sort(raleigh_infra_order_raw.begin(),
         raleigh_infra_order_raw.end(),
         by_xreal());

    /// Create catchments and corresponding vector
    vector<int> rof_triggered_infra_order_durham =
            vecInfraRankToVecInt(durham_infra_order_raw);
    vector<int> rof_triggered_infra_order_owasa =
            vecInfraRankToVecInt(owasa_infra_order_raw);
    vector<int> rof_triggered_infra_order_raleigh =
            vecInfraRankToVecInt(raleigh_infra_order_raw);

    added_storage_michie_expansion_high =
            checkAndFixInfraExpansionHighLowOrder(
                    &rof_triggered_infra_order_durham,
                    15,
                    16,
                    added_storage_michie_expansion_low,
                    added_storage_michie_expansion_high);

    reclaimed_capacity_high =
            checkAndFixInfraExpansionHighLowOrder(
                    &rof_triggered_infra_order_durham,
                    18,
                    19,
                    reclaimed_capacity_low,
                    reclaimed_capacity_high);


    /// Normalize Jordan Lake Allocations in case they exceed 1.
    double sum_jla_allocations = OWASA_JLA + Durham_JLA + Cary_JLA +
                                 Raleigh_JLA;
    if (sum_jla_allocations > 1.) {
        OWASA_JLA /= sum_jla_allocations;
        Durham_JLA /= sum_jla_allocations;
        Cary_JLA /= sum_jla_allocations;
        Raleigh_JLA /= sum_jla_allocations;
    }

    /// Normalize West Jordan Lake WTP allocations.
    double sum_wjlwtp = western_wake_treatment_frac_durham +
                        western_wake_treatment_plant_owasa_frac +
                        western_wake_treatment_plant_raleigh_frac;
    western_wake_treatment_frac_durham /= sum_wjlwtp;
    western_wake_treatment_plant_owasa_frac /= sum_wjlwtp;
    western_wake_treatment_plant_raleigh_frac /= sum_wjlwtp;


    // ===================== SET UP PROBLEM COMPONENTS =====================

    cout << "BEGINNING TRIANGLE TEST" << endl << endl;
    cout << "Using " << n_threads << " cores." << endl;
//    cout << getexepath() << endl;

    /// Read streamflows
    int streamflow_n_weeks = 52 * (70 + 50);
    /*
    vector<vector<double>> streamflows_durham = Utils::parse2DCsvFile("../TestFiles/durhamInflowsLong.csv");
    vector<vector<double>> streamflows_flat = Utils::parse2DCsvFile("../TestFiles/flatInflowsLong.csv");
    vector<vector<double>> streamflows_swift = Utils::parse2DCsvFile("../TestFiles/swiftInflowsLong.csv");
    vector<vector<double>> streamflows_llr = Utils::parse2DCsvFile("../TestFiles/littleriverraleighInflowsLong.csv");
    vector<vector<double>> streamflows_crabtree = Utils::parse2DCsvFile("../TestFiles/crabtreeInflowsLong.csv");
    vector<vector<double>> streamflows_phils = Utils::parse2DCsvFile("../TestFiles/philsInflowsLong.csv");
    vector<vector<double>> streamflows_cane = Utils::parse2DCsvFile("../TestFiles/caneInflowsLong.csv");
    vector<vector<double>> streamflows_morgan = Utils::parse2DCsvFile("../TestFiles/morganInflowsLong.csv");
    vector<vector<double>> streamflows_haw = Utils::parse2DCsvFile("../TestFiles/hawInflowsLong.csv");
    vector<vector<double>> streamflows_clayton = Utils::parse2DCsvFile("../TestFiles/claytonInflowsLong.csv");
    vector<vector<double>> streamflows_lillington = Utils::parse2DCsvFile("../TestFiles/lillingtonInflowsLong.csv");

    vector<vector<double>> demand_cary = Utils::parse2DCsvFile("../TestFiles/demandsLongCary.csv");
    vector<vector<double>> demand_durham = Utils::parse2DCsvFile("../TestFiles/demandsLongDurham.csv");
    vector<vector<double>> demand_raleigh = Utils::parse2DCsvFile("../TestFiles/demandsLongRaleigh.csv");
    vector<vector<double>> demand_owasa = Utils::parse2DCsvFile("../TestFiles/demandsLongOWASA.csv");

    vector<vector<double>> evap_durham = Utils::parse2DCsvFile("../TestFiles/evapLongDurham.csv");
    vector<vector<double>> evap_jordan_lake = Utils::parse2DCsvFile("../TestFiles/evapLongCary.csv");
    vector<vector<double>> evap_falls_lake = Utils::parse2DCsvFile("../TestFiles/evapLongFalls.csv");
    vector<vector<double>> evap_owasa = Utils::parse2DCsvFile("../TestFiles/evapLongOWASA.csv");
    vector<vector<double>> evap_little_river = Utils::parse2DCsvFile("../TestFiles/evapLongRaleighOther.csv");
    vector<vector<double>> evap_wheeler_benson = Utils::parse2DCsvFile("../TestFiles/evapLongWB.csv");

    vector<vector<double>> demand_to_wastewater_fraction_owasa_raleigh =
            Utils::parse2DCsvFile("../TestFiles/demand_to_wastewater_fraction_owasa_raleigh.csv");
    vector<vector<double>> demand_to_wastewater_fraction_durham =
            Utils::parse2DCsvFile("../TestFiles/demand_to_wastewater_fraction_durham.csv");

    vector<vector<double>> caryDemandClassesFractions = Utils::parse2DCsvFile
            ("../TestFiles/caryDemandClassesFractions.csv");
    vector<vector<double>> durhamDemandClassesFractions = Utils::parse2DCsvFile
            ("../TestFiles/durhamDemandClassesFractions.csv");
    vector<vector<double>> raleighDemandClassesFractions = Utils::parse2DCsvFile
            ("../TestFiles/raleighDemandClassesFractions.csv");
    vector<vector<double>> owasaDemandClassesFractions = Utils::parse2DCsvFile
            ("../TestFiles/owasaDemandClassesFractions.csv");

    vector<vector<double>> caryUserClassesWaterPrices = Utils::parse2DCsvFile
            ("../TestFiles/caryUserClassesWaterPrices.csv");
    vector<vector<double>> durhamUserClassesWaterPrices = Utils::parse2DCsvFile
            ("../TestFiles/durhamUserClassesWaterPrices.csv");
    vector<vector<double>> raleighUserClassesWaterPrices = Utils::parse2DCsvFile
            ("../TestFiles/raleighUserClassesWaterPrices.csv");
    vector<vector<double>> owasaUserClassesWaterPrices = Utils::parse2DCsvFile
            ("../TestFiles/owasaUserClassesWaterPrices.csv");

    vector<double> sewageFractions = Utils::parse1DCsvFile(
            "../TestFiles/sewageFractions.csv");
    */

//    vector<vector<double>> streamflows_durham = Utils::parse2DCsvFile(output_directory + "/TestFiles/durhamInflowsLong.csv");
//    vector<vector<double>> streamflows_flat = Utils::parse2DCsvFile(output_directory + "/TestFiles/flatInflowsLong.csv");
//    vector<vector<double>> streamflows_swift = Utils::parse2DCsvFile(output_directory + "/TestFiles/swiftInflowsLong.csv");
//    vector<vector<double>> streamflows_llr = Utils::parse2DCsvFile(output_directory + "/TestFiles/littleriverraleighInflowsLong.csv");
//    vector<vector<double>> streamflows_crabtree = Utils::parse2DCsvFile(output_directory + "/TestFiles/crabtreeInflowsLong.csv");
//    vector<vector<double>> streamflows_phils = Utils::parse2DCsvFile(output_directory + "/TestFiles/philsInflowsLong.csv");
//    vector<vector<double>> streamflows_cane = Utils::parse2DCsvFile(output_directory + "/TestFiles/caneInflowsLong.csv");
//    vector<vector<double>> streamflows_morgan = Utils::parse2DCsvFile(output_directory + "/TestFiles/morganInflowsLong.csv");
//    vector<vector<double>> streamflows_haw = Utils::parse2DCsvFile(output_directory + "/TestFiles/hawInflowsLong.csv");
//    vector<vector<double>> streamflows_clayton = Utils::parse2DCsvFile(output_directory + "/TestFiles/claytonInflowsLong.csv");
//    vector<vector<double>> streamflows_lillington = Utils::parse2DCsvFile(output_directory + "/TestFiles/lillingtonInflowsLong.csv");
//
//    vector<vector<double>> demand_cary = Utils::parse2DCsvFile(output_directory + "/TestFiles/demandsLongCary.csv");
//    vector<vector<double>> demand_durham = Utils::parse2DCsvFile(output_directory + "/TestFiles/demandsLongDurham.csv");
//    vector<vector<double>> demand_raleigh = Utils::parse2DCsvFile(output_directory + "/TestFiles/demandsLongRaleigh.csv");
//    vector<vector<double>> demand_owasa = Utils::parse2DCsvFile(output_directory + "/TestFiles/demandsLongOWASA.csv");
//
//    vector<vector<double>> evap_durham = Utils::parse2DCsvFile(output_directory + "/TestFiles/evapLongDurham.csv");
//    vector<vector<double>> evap_jordan_lake = Utils::parse2DCsvFile(output_directory + "/TestFiles/evapLongCary.csv");
//    vector<vector<double>> evap_falls_lake = Utils::parse2DCsvFile(output_directory + "/TestFiles/evapLongFalls.csv");
//    vector<vector<double>> evap_owasa = Utils::parse2DCsvFile(output_directory + "/TestFiles/evapLongOWASA.csv");
//    vector<vector<double>> evap_little_river = Utils::parse2DCsvFile(output_directory + "/TestFiles/evapLongRaleighOther.csv");
//    vector<vector<double>> evap_wheeler_benson = Utils::parse2DCsvFile(output_directory + "/TestFiles/evapLongWB.csv");

    int max_lines = n_realizations;

    cout << "Reading inflows." << endl;
    vector<vector<double>> streamflows_durham = Utils::parse2DCsvFile(output_directory + "/TestFiles/inflows/durham_inflows.csv",
                                                                      max_lines);
    vector<vector<double>> streamflows_flat = Utils::parse2DCsvFile(output_directory + "/TestFiles/inflows/falls_lake_inflows.csv",
                                                                    max_lines);
    vector<vector<double>> streamflows_swift = Utils::parse2DCsvFile(output_directory + "/TestFiles/inflows/lake_wb_inflows.csv",
                                                                     max_lines);
    vector<vector<double>> streamflows_llr = Utils::parse2DCsvFile(output_directory + "/TestFiles/inflows/little_river_raleigh_inflows.csv",
                                                                   max_lines);
    vector<vector<double>> streamflows_crabtree = Utils::parse2DCsvFile(output_directory + "/TestFiles/inflows/crabtree_inflows.csv",
                                                                        max_lines);

    vector<vector<double>> streamflows_phils = Utils::parse2DCsvFile(output_directory + "/TestFiles/inflows/stone_quarry_inflows.csv",
                                                                     max_lines);
    vector<vector<double>> streamflows_cane = Utils::parse2DCsvFile(output_directory + "/TestFiles/inflows/cane_creek_inflows.csv",
                                                                    max_lines);
    vector<vector<double>> streamflows_morgan = Utils::parse2DCsvFile(output_directory + "/TestFiles/inflows/university_lake_inflows.csv",
                                                                      max_lines);
    vector<vector<double>> streamflows_haw = Utils::parse2DCsvFile(output_directory + "/TestFiles/inflows/jordan_lake_inflows.csv",
                                                                   max_lines);
    vector<vector<double>> streamflows_clayton = Utils::parse2DCsvFile(output_directory + "/TestFiles/inflows/clayton_inflows.csv",
                                                                       max_lines);

    vector<vector<double>> streamflows_lillington = Utils::parse2DCsvFile(output_directory + "/TestFiles/inflows/lillington_inflows.csv",
                                                                          max_lines);

    cout << "Reading demands." << endl;
    vector<vector<double>> demand_cary = Utils::parse2DCsvFile(output_directory + "/TestFiles/demands/cary_demand.csv",
                                                               max_lines);
    vector<vector<double>> demand_durham = Utils::parse2DCsvFile(output_directory + "/TestFiles/demands/durham_demand.csv",
                                                                 max_lines);
    vector<vector<double>> demand_raleigh = Utils::parse2DCsvFile(output_directory + "/TestFiles/demands/raleigh_demand.csv",
                                                                  max_lines);
    vector<vector<double>> demand_owasa = Utils::parse2DCsvFile(output_directory + "/TestFiles/demands/owasa_demand.csv",
                                                                max_lines);

    cout << "Reading evaporations." << endl;
    vector<vector<double>> evap_durham = Utils::parse2DCsvFile(output_directory + "/TestFiles/evaporation/durham_evap.csv",
                                                               max_lines);
    vector<vector<double>> evap_falls_lake = Utils::parse2DCsvFile(output_directory + "/TestFiles/evaporation/falls_lake_evap.csv",
                                                                   max_lines);
    vector<vector<double>> evap_owasa = Utils::parse2DCsvFile(output_directory + "/TestFiles/evaporation/owasa_evap.csv",
                                                              max_lines);
    vector<vector<double>> evap_little_river = Utils::parse2DCsvFile(output_directory + "/TestFiles/evaporation/little_river_raleigh_evap.csv",
                                                                     max_lines);
    vector<vector<double>> evap_wheeler_benson = Utils::parse2DCsvFile(output_directory + "/TestFiles/evaporation/wb_evap.csv",
                                                                       max_lines);
    vector<vector<double>> evap_jordan_lake = evap_owasa;

    cout << "Reading others." << endl;
    vector<vector<double>> demand_to_wastewater_fraction_owasa_raleigh =
            Utils::parse2DCsvFile(output_directory + "/TestFiles/demand_to_wastewater_fraction_owasa_raleigh.csv");
    vector<vector<double>> demand_to_wastewater_fraction_durham =
            Utils::parse2DCsvFile(output_directory + "/TestFiles/demand_to_wastewater_fraction_durham.csv");
//
    vector<vector<double>> caryDemandClassesFractions = Utils::parse2DCsvFile
            (output_directory + "/TestFiles/caryDemandClassesFractions.csv");
    vector<vector<double>> durhamDemandClassesFractions = Utils::parse2DCsvFile
            (output_directory + "/TestFiles/durhamDemandClassesFractions.csv");
    vector<vector<double>> raleighDemandClassesFractions = Utils::parse2DCsvFile
            (output_directory + "/TestFiles/raleighDemandClassesFractions.csv");
    vector<vector<double>> owasaDemandClassesFractions = Utils::parse2DCsvFile
            (output_directory + "/TestFiles/owasaDemandClassesFractions.csv");

    vector<vector<double>> caryUserClassesWaterPrices = Utils::parse2DCsvFile
            (output_directory + "/TestFiles/caryUserClassesWaterPrices.csv");
    vector<vector<double>> durhamUserClassesWaterPrices = Utils::parse2DCsvFile
            (output_directory + "/TestFiles/durhamUserClassesWaterPrices.csv");
    vector<vector<double>> raleighUserClassesWaterPrices = Utils::parse2DCsvFile
            (output_directory + "/TestFiles/raleighUserClassesWaterPrices.csv");
    vector<vector<double>> owasaUserClassesWaterPrices = Utils::parse2DCsvFile
            (output_directory + "/TestFiles/owasaUserClassesWaterPrices.csv");

    vector<vector<double>> owasaPriceSurcharges = Utils::parse2DCsvFile
            (output_directory + "/TestFiles/owasaPriceRestMultipliers.csv");

//    vector<double> sewageFractions = Utils::parse1DCsvFile(
//            output_directory + "/TestFiles/sewageFractions.csv");

    EvaporationSeries evaporation_durham(&evap_durham, streamflow_n_weeks);
    EvaporationSeries evaporation_jordan_lake(
            &evap_jordan_lake,
            streamflow_n_weeks);
    EvaporationSeries evaporation_falls_lake(
            &evap_falls_lake,
            streamflow_n_weeks);
    EvaporationSeries evaporation_owasa(&evap_owasa, streamflow_n_weeks);
    EvaporationSeries evaporation_little_river(
            &evap_little_river,
            streamflow_n_weeks);
    EvaporationSeries evaporation_wheeler_benson(
            &evap_wheeler_benson,
            streamflow_n_weeks);

    /// Create catchments and corresponding vectors
    //  Durham (Upper Neuse River Basin)
    Catchment durham_inflows(&streamflows_durham, streamflow_n_weeks);

    //  Raleigh (Lower Neuse River Basin)
    Catchment lower_flat_river(&streamflows_flat, streamflow_n_weeks);
    Catchment swift_creek(&streamflows_swift, streamflow_n_weeks);
    Catchment little_river_raleigh(&streamflows_llr, streamflow_n_weeks);
    Catchment crabtree_creek(&streamflows_crabtree, streamflow_n_weeks);

    // OWASA (Upper Cape Fear Basin)
    Catchment phils_reek(&streamflows_phils, streamflow_n_weeks);
    Catchment cane_creek(&streamflows_cane, streamflow_n_weeks);
    Catchment morgan_creek(&streamflows_morgan, streamflow_n_weeks);

    // Cary (Lower Cape Fear Basin)
    Catchment lower_haw_river(&streamflows_haw, streamflow_n_weeks);

    // Downstream Gages
    Catchment neuse_river_at_clayton(&streamflows_clayton, streamflow_n_weeks);
    Catchment cape_fear_river_at_lillington(
            &streamflows_lillington,
            streamflow_n_weeks);

    vector<Catchment *> catchment_durham;

    vector<Catchment *> catchment_flat;
    vector<Catchment *> catchment_swift;
    vector<Catchment *> catchment_little_river_raleigh;
    vector<Catchment *> catchment_crabtree;

    vector<Catchment *> catchment_phils;
    vector<Catchment *> catchment_cane;
    vector<Catchment *> catchment_morgan;

    vector<Catchment *> catchment_haw;

    vector<Catchment *> gage_clayton;
    vector<Catchment *> gage_lillington;

    catchment_durham.push_back(&durham_inflows);

    catchment_flat.push_back(&lower_flat_river);
    catchment_swift.push_back(&swift_creek);
    catchment_little_river_raleigh.push_back(&little_river_raleigh);
    catchment_crabtree.push_back(&crabtree_creek);

    catchment_phils.push_back(&phils_reek);
    catchment_cane.push_back(&cane_creek);
    catchment_morgan.push_back(&morgan_creek);

    catchment_haw.push_back(&lower_haw_river);

    gage_clayton.push_back(&neuse_river_at_clayton);
    gage_lillington.push_back(&cape_fear_river_at_lillington);

    /// Storage vs. area reservoir curves.
    vector<double> falls_lake_storage = {0, 23266, 34700};
    vector<double> falls_lake_area = {0.32 * 5734, 0.32 * 29000, 0.28 * 40434};
    vector<double> wheeler_benson_storage = {0, 2789.66};
    vector<double> wheeler_benson_area = {0, 0.3675 * 2789.66};
    vector<double> teer_storage = {0, 1315.0};
    vector<double> teer_area = {20, 50};
    vector<double> little_river_res_storage = {0, 3700};
    vector<double> little_river_res_area = {0, 0.3675 * 3700};

    DataSeries falls_lake_storage_area(&falls_lake_storage, &falls_lake_area);
    DataSeries wheeler_benson_storage_area(&wheeler_benson_storage,
                                           &wheeler_benson_area);
    DataSeries teer_storage_area(&teer_storage,
                                 &teer_area);
    DataSeries little_river_storage_area(&little_river_res_storage,
                                         &little_river_res_area);

    /// Minimum environmental flow rules (controls)
    vector<int> dlr_weeks = {0, 21, 47, 53};
    vector<double> dlr_releases = {3.877 * 7, 9.05, 3.877 * 7};
    vector<double> wb_storage = {0.3 * 2789.66, 0.6 * 2789.66, 2789.66};
    vector<double> wb_releases = {0.646 * 7, 1.29 * 7, 1.94 * 7};
    vector<double> ccr_inflows = {0.1422 * 7, 0.5 * 7, 1 * 7, 1.5 * 7,
                                  1.797 * 7};
    vector<double> ccr_releases = {0.1422 * 7, 0.5 * 7, 1 * 7, 1.5 * 7,
                                   1.797 * 7};
    int falls_controls_weeks[2] = {13, 43};
    double falls_base_releases[2] = {64.64 * 7, 38.78 * 7};
    double falls_min_gage[2] = {180 * 7, 130 * 7};

    SeasonalMinEnvFlowControl durham_min_env_control(0, &dlr_weeks,
                                                     &dlr_releases);
//    FixedMinEnvFlowControl falls_min_env_control(1, 38.78 * 7);
    FallsLakeMinEnvFlowControl falls_min_env_control(1,
                                                     10,
                                                     falls_controls_weeks,
                                                     falls_base_releases,
                                                     falls_min_gage,
                                                     crabtree_creek);

    StorageMinEnvFlowControl wheeler_benson_min_env_control(2,
                                                            vector<int>(1,
                                                                        2),
                                                            &wb_storage,
                                                            &wb_releases);
    FixedMinEnvFlowControl sq_min_env_control(3, 0);
    InflowMinEnvFlowControl ccr_min_env_control(4, vector<int>(1, 4),
                                                &ccr_inflows,
                                                &ccr_releases);
    FixedMinEnvFlowControl university_min_env_control(5, 0);
//    FixedMinEnvFlowControl jordan_min_env_control(6,
//                                                  25.8527 * 7);
    JordanLakeMinEnvFlowControl jordan_min_env_control(
            6, &cape_fear_river_at_lillington, 64.63, 129.26, 25.85, 193.89,
            290.84, 387.79, 30825.0, 14924.0);
    SeasonalMinEnvFlowControl little_river_min_env_control(7, &dlr_weeks,
                                                           &dlr_releases);
    FixedMinEnvFlowControl richland_min_env_control(8, 0);
    FixedMinEnvFlowControl teer_min_env_control(9, 0);
    FixedMinEnvFlowControl neuse_intake_min_env_control(10, 38.78 * 7);

//    vector<int> eno_weeks = {7, 16, 53};
//    vector<double> eno_releases = {6.49 * 7, 19.48 * 7, 6.49 * 7};
//    SeasonalMinEnvFlowControl eno_min_env_control(&eno_weeks, &eno_releases);

    vector<MinEnvironFlowControl *> min_env_flow_controls;
    min_env_flow_controls.push_back(&durham_min_env_control);
    min_env_flow_controls.push_back(&falls_min_env_control);
    min_env_flow_controls.push_back(&wheeler_benson_min_env_control);
    min_env_flow_controls.push_back(&sq_min_env_control);
    min_env_flow_controls.push_back(&ccr_min_env_control);
    min_env_flow_controls.push_back(&university_min_env_control);
    min_env_flow_controls.push_back(&jordan_min_env_control);
    min_env_flow_controls.push_back(&little_river_min_env_control);
    min_env_flow_controls.push_back(&richland_min_env_control);
    min_env_flow_controls.push_back(&teer_min_env_control);
    min_env_flow_controls.push_back(&neuse_intake_min_env_control);

    /// Create reservoirs and corresponding vector
    vector<double> construction_time_interval = {3.0, 5.0};
    vector<double> city_infrastructure_rof_triggers = {owasa_inftrigger,
                                                       durham_inftrigger,
                                                       cary_inftrigger,
                                                       raleigh_inftrigger};
    vector<double> bond_length = {25, 25, 25, 25};
    vector<double> bond_rate = {0.05, 0.05, 0.05, 0.05};

    /// Jordan Lake parameters
    double jl_supply_capacity = 14924.0;
    double jl_wq_capacity = 30825.0;
    double jl_storage_capacity = jl_wq_capacity + jl_supply_capacity;
    vector<int> jl_allocations_ids = {0, 1, 2, 3, WATER_QUALITY_ALLOCATION};
    vector<double> jl_allocation_fractions = {
            OWASA_JLA * jl_supply_capacity / jl_storage_capacity,
            Durham_JLA * jl_supply_capacity / jl_storage_capacity,
            Cary_JLA * jl_supply_capacity / jl_storage_capacity,
            Raleigh_JLA * jl_supply_capacity / jl_storage_capacity,
            jl_wq_capacity / jl_storage_capacity};
    vector<double> jl_treatment_allocation_fractions = {0.0, 0.0, 1.0, 0.0};

    /// Jordan Lake parameters
    double fl_supply_capacity = 14700.0;
    double fl_wq_capacity = 20000.0;
    double fl_storage_capacity = fl_wq_capacity + fl_supply_capacity;
    vector<int> fl_allocations_ids = {3, WATER_QUALITY_ALLOCATION};
    vector<double> fl_allocation_fractions = {
            fl_supply_capacity / fl_storage_capacity,
            fl_wq_capacity / fl_storage_capacity};
    vector<double> fl_treatment_allocation_fractions = {0.0, 0.0, 0.0, 1.0};

    // Existing Sources
    Reservoir durham_reservoirs("Lake Michie & Little River Res. (Durham)",
                                0,
                                catchment_durham,
                                6349.0,
                                ILLIMITED_TREATMENT_CAPACITY,
                                &evaporation_durham, 1069);
//    Reservoir falls_lake("Falls Lake", 1, catchment_flat,
//                         34700.0, 99999,
//                         &evaporation_falls_lake, &falls_lake_storage_area);
    AllocatedReservoir falls_lake("Falls Lake",
                                  1,
                                  catchment_flat,
                                  fl_storage_capacity,
                                  ILLIMITED_TREATMENT_CAPACITY,
                                  &evaporation_falls_lake,
                                  &falls_lake_storage_area,
                                  &fl_allocations_ids,
                                  &fl_allocation_fractions,
                                  &fl_treatment_allocation_fractions);

    Reservoir wheeler_benson_lakes("Wheeler-Benson Lakes", 2, catchment_swift,
                                   2789.66,
                                   ILLIMITED_TREATMENT_CAPACITY,
                                   &evaporation_wheeler_benson,
                                   &wheeler_benson_storage_area);
    Reservoir stone_quarry("Stone Quarry",
                           3,
                           catchment_phils,
                           200.0,
                           ILLIMITED_TREATMENT_CAPACITY,
                           &evaporation_owasa,
                           10);
    Reservoir ccr("Cane Creek Reservoir",
                  4,
                  catchment_cane,
                  2909.0,
                  ILLIMITED_TREATMENT_CAPACITY,
                  &evaporation_owasa,
                  500);
    Reservoir university_lake("University Lake", 5, catchment_morgan, 449.0,
                              ILLIMITED_TREATMENT_CAPACITY,
                              &evaporation_owasa,
                              212);
//    Reservoir jordan_lake("Jordan Lake", 6, catchment_haw,
//                          jl_supply_capacity, 448,
//                          &evaporation_jordan_lake, 13940);
    AllocatedReservoir jordan_lake("Jordan Lake",
                                   6,
                                   catchment_haw,
                                   jl_storage_capacity,
                                   448,
                                   &evaporation_jordan_lake,
                                   13940,
                                   &jl_allocations_ids,
                                   &jl_allocation_fractions,
                                   &jl_treatment_allocation_fractions);

    // other than Cary WTP for Jordan Lake, assume no WTP constraints - each
    // city can meet its daily demands with available treatment infrastructure

    double WEEKS_IN_YEAR = 0;

    // Potential Sources
    // The capacities listed here for expansions are what additional capacity is gained relative to existing capacity,
    // NOT the total capacity after the expansion is complete. For infrastructure with a high and low option, this means
    // the capacity for both is relative to current conditions - if Lake Michie is expanded small it will gain 2.5BG,
    // and a high expansion will provide 7.7BG more water than current. if small expansion happens, followed by a large
    // expansion, the maximum capacity through expansion is 7.7BG, NOT 2.5BG + 7.7BG.
    Reservoir little_river_reservoir("Little River Reservoir (Raleigh)", 7,
                                     catchment_little_river_raleigh, 3700.0,
                                     ILLIMITED_TREATMENT_CAPACITY,
                                     &evaporation_little_river,
                                     &little_river_storage_area,
                                     city_infrastructure_rof_triggers[3],
                                     construction_time_interval,
                                     17 *
                                     WEEKS_IN_YEAR,
                                     263.0,
                                     bond_length[3], bond_rate[3]);
    Quarry richland_creek_quarry("Richland Creek Quarry", 8, gage_clayton,
                                 4000.0,
                                 ILLIMITED_TREATMENT_CAPACITY,
                                 &evaporation_falls_lake,
                                 100.,
                                 city_infrastructure_rof_triggers[3],
                                 construction_time_interval,
                                 17 *
                                 WEEKS_IN_YEAR,
                                 400.0,
                                 bond_length[3],
                                 bond_rate[3],
                                 50 * 7);
    // diversions to Richland Creek Quarry based on ability to meet downstream flow target at Clayton, NC
    Quarry teer_quarry("Teer Quarry",
                       9,
                       vector<Catchment *>(),
                       1315.0,
                       ILLIMITED_TREATMENT_CAPACITY,
                       &evaporation_falls_lake,
                       &teer_storage_area,
                       city_infrastructure_rof_triggers[1],
                       construction_time_interval,
                       7 *
                       WEEKS_IN_YEAR,
                       22.6,
                       bond_length[1],
                       bond_rate[1],
                       15 * 7); //FIXME: MAX PUMPING CAPACITY?
    //Reservoir rNeuseRiverIntake("rNeuseRiverIntake", 10, 0, catchment_flat, 16.0, 99999, city_infrastructure_rof_triggers[3], construction_time_interval, 5000, 20, 0.05);
    Intake neuse_river_intake("Neuse River Intake", 10, catchment_flat, 16 * 7,
                              city_infrastructure_rof_triggers[3],
                              construction_time_interval,
                              17 * WEEKS_IN_YEAR,
                              225.5,
                              bond_length[3],
                              bond_rate[3]);
    // diversions to Teer Quarry for Durham based on inflows to downstream Falls Lake from the Flat River
    // FYI: spillage from Eno River also helps determine Teer quarry diversion, but Eno spillage isn't factored into
    // downstream water balance?

    vector<double> fl_relocation_fractions = {
            (fl_supply_capacity + falls_lake_reallocation) /
            fl_storage_capacity,
            (fl_wq_capacity - falls_lake_reallocation) / fl_storage_capacity};
    Relocation fl_reallocation("Falls Lake Reallocation",
                               17,
                               1,
                               &fl_relocation_fractions,
                               &fl_allocations_ids,
                               city_infrastructure_rof_triggers[3],
                               construction_time_interval,
                               12 * WEEKS_IN_YEAR,
                               68.2,
                               bond_length[3],
                               bond_rate[3]);
    ReservoirExpansion ccr_expansion("Cane Creek Reservoir Expansion",
                                     24,
                                     4,
                                     3000.0,
                                     city_infrastructure_rof_triggers[2],
                                     construction_time_interval,
                                     17 * WEEKS_IN_YEAR,
                                     127.0,
                                     bond_length[2], bond_rate[2]);
    ReservoirExpansion low_sq_expansion("Low Stone Quarry Expansion", 12, 3,
                                        1500.0,
                                        city_infrastructure_rof_triggers[2],
                                        construction_time_interval,
                                        23 * WEEKS_IN_YEAR,
                                        1.4,
                                        bond_length[2], bond_rate[2]);
    ReservoirExpansion high_sq_expansion("High Stone Quarry Expansion", 13, 3,
                                         2200.0,
                                         city_infrastructure_rof_triggers[2],
                                         construction_time_interval,
                                         23 * WEEKS_IN_YEAR,
                                         64.6,
                                         bond_length[2], bond_rate[2]);
    ReservoirExpansion univ_lake_expansion("University Lake Expansion",
                                           14,
                                           5,
                                           2550.0,
                                           city_infrastructure_rof_triggers[2],
                                           construction_time_interval,
                                           17 * WEEKS_IN_YEAR,
                                           107.0,
                                           bond_length[2], bond_rate[2]);
    ReservoirExpansion low_michie_expansion("Low Lake Michie Expansion",
                                            15,
                                            0,
                                            added_storage_michie_expansion_low,
                                            city_infrastructure_rof_triggers[1],
                                            construction_time_interval,
                                            17 * WEEKS_IN_YEAR,
                                            158.3,
                                            bond_length[1],
                                            bond_rate[1]);
    ReservoirExpansion high_michie_expansion("High Lake Michie Expansion",
                                             16,
                                             0,
                                             added_storage_michie_expansion_high,
                                             city_infrastructure_rof_triggers[1],
                                             construction_time_interval,
                                             17 * WEEKS_IN_YEAR,
                                             203.3,
                                             bond_length[1], bond_rate[1]);
    WaterReuse low_reclaimed("Low Reclaimed Water System",
                             18,
                             reclaimed_capacity_low,
                             city_infrastructure_rof_triggers[1],
                             construction_time_interval,
                             7 * WEEKS_IN_YEAR,
                             27.5,
                             bond_length[1],
                             bond_rate[1]);
    WaterReuse high_reclaimed("High Reclaimed Water System",
                              19,
                              reclaimed_capacity_high,
                              city_infrastructure_rof_triggers[1],
                              construction_time_interval,
                              7 * WEEKS_IN_YEAR,
                              104.4,
                              bond_length[1],
                              bond_rate[1]);

    WEEKS_IN_YEAR = Constants::WEEKS_IN_YEAR;

    vector<double> wjlwtp_treatment_capacity_fractions =
            {western_wake_treatment_plant_owasa_frac,
             western_wake_treatment_frac_durham,
             0.,
             western_wake_treatment_plant_raleigh_frac,
             0.};
    vector<double> cary_upgrades_treatment_capacity_fractions = {0., 0., 1.,
                                                                 0., 0.};

    vector<double> *shared_added_wjlwtp_treatment_pool = new vector<double>();
    vector<double> *shared_added_wjlwtp_price = new vector<double>();
    SequentialJointTreatmentExpansion low_wjlwtp("Low WJLWTP",
                                                 20,
                                                 6,
                                                 0,
                                                 33 * 7,
                                                 &wjlwtp_treatment_capacity_fractions,
                                                 shared_added_wjlwtp_treatment_pool,
                                                 shared_added_wjlwtp_price,
                                                 city_infrastructure_rof_triggers[1],
                                                 construction_time_interval,
                                                 12 * WEEKS_IN_YEAR,
                                                 243.3,
                                                 bond_length[1],
                                                 bond_rate[1]);
    SequentialJointTreatmentExpansion high_wjlwtp("High WJLWTP",
                                                  21,
                                                  6,
                                                  0,
                                                  54 * 7,
                                                  &wjlwtp_treatment_capacity_fractions,
                                                  shared_added_wjlwtp_treatment_pool,
                                                  shared_added_wjlwtp_price,
                                                  city_infrastructure_rof_triggers[1],
                                                  construction_time_interval,
                                                  12 * WEEKS_IN_YEAR,
                                                  316.8,
                                                  bond_length[1],
                                                  bond_rate[1]);
    SequentialJointTreatmentExpansion caryWtpUpgrade1("Cary WTP upgrade 1",
                                                      22,
                                                      6,
                                                      56, // (72*7 - 448 = 56)
                                             &cary_upgrades_treatment_capacity_fractions,
                                             caryupgrades_2 * 7,
                                                      construction_time_interval,
                                                      NONE,
                                                      243. / 2,
                                                      bond_length[1],
                                                      bond_rate[1]);
    SequentialJointTreatmentExpansion caryWtpUpgrade2("Cary WTP upgrade 2",
                                                      23,
                                                      6,
                                                      56, // (7*80 - 72*7 = 56)
                                             &cary_upgrades_treatment_capacity_fractions,
                                             caryupgrades_3 * 7,
                                                      construction_time_interval,
                                                      NONE,
                                                      316.8 / 2,
                                                      bond_length[1],
                                                      bond_rate[1]);
//    Reservoir low_wjlwtp_durham("Low WJLWTP (Durham)", 20, 0, catchment_haw,
//                                jl_storage_capacity * JL_allocation_fractions[1], 33.0 * JL_allocation_fractions[1],
//                                &evaporation_jordan_lake, 13940,
//                                city_infrastructure_rof_triggers[1], construction_time_interval,
//                                243.3 * JL_allocation_fractions[1], bond_length[1], bond_rate[1]);
//    Reservoir high_wjlwtp_durham("High WJLWTP (Durham)", 21, 0, catchment_haw,
//                                 jl_storage_capacity * JL_allocation_fractions[1], 54.0 * JL_allocation_fractions[1],
//                                 &evaporation_jordan_lake, 13940,
//                                 city_infrastructure_rof_triggers[1], construction_time_interval,
//                                 73.5 * JL_allocation_fractions[1], bond_length[1], bond_rate[1]);
//    Reservoir low_wjlwtp_owasa("Low WJLWTP (OWASA)", 22, 0, catchment_haw,
//                               jl_storage_capacity * JL_allocation_fractions[2], 33.0 * JL_allocation_fractions[2],
//                               &evaporation_jordan_lake, 13940,
//                               city_infrastructure_rof_triggers[2], construction_time_interval,
//                               243.3 * JL_allocation_fractions[2], bond_length[2], bond_rate[2]);
//    Reservoir high_wjlwtp_owasa("High WJLWTP (OWASA)", 23, 0, catchment_haw,
//                                jl_storage_capacity * JL_allocation_fractions[2], 54.0 * JL_allocation_fractions[2],
//                                &evaporation_jordan_lake, 13940,
//                                city_infrastructure_rof_triggers[2], construction_time_interval,
//                                73.5 * JL_allocation_fractions[2], bond_length[2], bond_rate[2]);
//    Reservoir low_wjlwtp_raleigh("Low WJLWTP (Raleigh)", 24, 0, catchment_haw,
//                                 jl_storage_capacity * JL_allocation_fractions[3], 33.0 * JL_allocation_fractions[3],
//                                 &evaporation_jordan_lake, 13940,
//                                 city_infrastructure_rof_triggers[3], construction_time_interval,
//                                 243.3 * JL_allocation_fractions[3], bond_length[3], bond_rate[3]);
//    Reservoir high_wjlwtp_raleigh("High WJLWTP (Raleigh)", 25, 0, catchment_haw,
//                                  jl_storage_capacity * JL_allocation_fractions[4], 54.0 * JL_allocation_fractions[3],
//                                  &evaporation_jordan_lake, 13940,
//                                  city_infrastructure_rof_triggers[3], construction_time_interval,
//                                  73.5 * JL_allocation_fractions[3], bond_length[3], bond_rate[3]);

    Reservoir dummy_endpoint("Dummy Node", 11, vector<Catchment *>(), 0, 0,
                             &evaporation_durham, 1, 1,
                             construction_time_interval,
                             0,
                             0,
                             0,
                             0);

    vector<WaterSource *> water_sources;
    water_sources.push_back(&durham_reservoirs);
    water_sources.push_back(&falls_lake);
    water_sources.push_back(&wheeler_benson_lakes);
    water_sources.push_back(&stone_quarry);
    water_sources.push_back(&ccr);
    water_sources.push_back(&university_lake);
    water_sources.push_back(&jordan_lake);

    water_sources.push_back(&little_river_reservoir);
    water_sources.push_back(&richland_creek_quarry);
    water_sources.push_back(&teer_quarry);
    water_sources.push_back(&fl_reallocation);
    water_sources.push_back(&ccr_expansion);
    water_sources.push_back(&univ_lake_expansion);
    water_sources.push_back(&neuse_river_intake);
    water_sources.push_back(&low_sq_expansion);
    water_sources.push_back(&low_michie_expansion);
    water_sources.push_back(&low_reclaimed);
    water_sources.push_back(&high_sq_expansion);
    water_sources.push_back(&high_michie_expansion);
    water_sources.push_back(&high_reclaimed);

    water_sources.push_back(&caryWtpUpgrade1);
    water_sources.push_back(&caryWtpUpgrade2);
    water_sources.push_back(&low_wjlwtp);
    water_sources.push_back(&high_wjlwtp);

    water_sources.push_back(&dummy_endpoint);

    /*
     * System connection diagram (water
     * flows from top to bottom)
     * Potential projects and expansions
     * of existing sources in parentheses
     *
     *      3(12,13)   4(24)   5(14)            0(15,16)    (18,19)
     *         \         /      /                  |
     *          \       /      /                   |
     *           \     /      /                    |
     *           |    /      /                    (9)
     *           |   /      /                      \
     *           |   |     /                        \
     *           |   |    /                          1(17)              2   (7)
     *           |   |   /                             |                |    |
     *           |   |  /                              |                |    |
     *           6(20-25)                             (8)               |    |
     *              |                                   \               |    |
     *              |                                    \              |    |
     *        Lillington Gage                            (10)           |    |
     *              |                                      |            |    |
     *              |                                      |           /    /
     *              |                                      |          /    /
     *              |                                 Clayton Gage   /    /
     *              |                                      |        /    /
     *               \                                     |   -----    /
     *                \                                     \ /        /
     *                 \                                     |    -----
     *                  \                                    |   /
     *                   \                                   |  /
     *                    \                                   \/
     *                     -------                            /
     *                            \             --------------
     *                             \           /
     *                              \     -----
     *                               \   /
     *                                \ /
     *                                11
     */

    Graph g(12);
    g.addEdge(0, 9);
    g.addEdge(9, 1);
    g.addEdge(1, 8);
    g.addEdge(8, 10);
    g.addEdge(10, 11);
    g.addEdge(2, 11);
    g.addEdge(7, 11);

    g.addEdge(3, 6);
    g.addEdge(4, 6);
    g.addEdge(5, 6);
    g.addEdge(6, 11);

    vector<int> demand_triggered_infra_order_cary = {22, 23};

    int demand_n_weeks = (int) round(46 * WEEKS_IN_YEAR);

    vector<int> cary_ws_return_id = {};
    vector<vector<double>> *cary_discharge_fraction_series =
            new vector<vector<double>>();
    WwtpDischargeRule wwtp_discharge_cary(
            cary_discharge_fraction_series,
            &cary_ws_return_id);
    vector<int> owasa_ws_return_id = {6};
    WwtpDischargeRule wwtp_discharge_owasa(
            &demand_to_wastewater_fraction_owasa_raleigh,
            &owasa_ws_return_id);
    vector<int> raleigh_ws_return_id = {8};
    WwtpDischargeRule wwtp_discharge_raleigh(
            &demand_to_wastewater_fraction_owasa_raleigh,
            &raleigh_ws_return_id);
    vector<int> durham_ws_return_id = {1, 6};
    WwtpDischargeRule wwtp_discharge_durham(
            &demand_to_wastewater_fraction_durham,
            &durham_ws_return_id);

    vector<vector<int>> wjlwtp_remove_from_to_build_list;// = {{21, 20}};

    Utility cary((char *) "Cary",
                 2,
                 &demand_cary,
                 demand_n_weeks,
                 cary_annual_payment,
                 &caryDemandClassesFractions,
                 &caryUserClassesWaterPrices,
                 wwtp_discharge_cary,
                 cary_inf_buffer,
                 vector<int>(),
                 demand_triggered_infra_order_cary,
                 0.05);
    Utility durham((char *) "Durham",
                   1,
                   &demand_durham,
                   demand_n_weeks,
                   durham_annual_payment,
                   &durhamDemandClassesFractions,
                   &durhamUserClassesWaterPrices,
                   wwtp_discharge_durham,
                   durham_inf_buffer,
                   rof_triggered_infra_order_durham,
                   vector<int>(),
                   0.05,
                   &wjlwtp_remove_from_to_build_list);
    Utility owasa((char *) "OWASA",
                  0,
                  &demand_owasa,
                  demand_n_weeks,
                  owasa_annual_payment,
                  &owasaDemandClassesFractions,
                  &owasaUserClassesWaterPrices,
                  wwtp_discharge_owasa,
                  owasa_inf_buffer,
                  rof_triggered_infra_order_owasa,
                  vector<int>(),
                  0.05,
                  &wjlwtp_remove_from_to_build_list);
    Utility raleigh((char *) "Raleigh",
                    3,
                    &demand_raleigh,
                    demand_n_weeks,
                    raleigh_annual_payment,
                    &raleighDemandClassesFractions,
                    &raleighUserClassesWaterPrices,
                    wwtp_discharge_raleigh,
                    raleigh_inf_buffer,
                    rof_triggered_infra_order_raleigh,
                    vector<int>(),
                    0.05,
                    &wjlwtp_remove_from_to_build_list);

    vector<Utility *> utilities;
    utilities.push_back(&cary);
    utilities.push_back(&durham);
    utilities.push_back(&owasa);
    utilities.push_back(&raleigh);

    /// Water-source-utility connectivity matrix (each row corresponds to a utility and numbers are water
    /// sources IDs.
    vector<vector<int>> reservoir_utility_connectivity_matrix = {
//            {6},
//            {0, 9, 15, 16, 18, 19, 20, 21},
//            {3, 4, 5,  26, 12, 13, 14, 22, 23, 20, 21},
//            {1, 2, 7,  8,  17, 10, 24, 25, 20, 21}
            {3, 4,  5, 6,  12, 13, 14, 20, 21, 24}, //OWASA
            {0, 6,  9, 15, 16, 18, 19, 20, 21}, //Durham
            {6, 22, 23},                    //Cary
            {1, 2,  6, 7,  8,  17, 10, 20, 21}  //Raleigh
    };

    vector<DroughtMitigationPolicy *> drought_mitigation_policies;
    /// Restriction policies
    vector<double> initial_restriction_triggers = {OWASA_restriction_trigger,
                                                   Durham_restriction_trigger,
                                                   cary_restriction_trigger,
                                                   raleigh_restriction_trigger};

    vector<double> restriction_stage_multipliers_cary = {0.9, 0.8, 0.7, 0.6};
    vector<double> restriction_stage_triggers_cary = {initial_restriction_triggers[0],
                                                      initial_restriction_triggers[0] + 0.15,
                                                      initial_restriction_triggers[0] + 0.35,
                                                      initial_restriction_triggers[0] + 0.6};
    vector<double> restriction_stage_multipliers_durham = {0.9, 0.8, 0.7, 0.6};
    vector<double> restriction_stage_triggers_durham = {initial_restriction_triggers[1],
                                                        initial_restriction_triggers[1] + 0.15,
                                                        initial_restriction_triggers[1] + 0.35,
                                                        initial_restriction_triggers[1] + 0.6};
    vector<double> restriction_stage_multipliers_owasa = {0.9, 0.8, 0.7};
    vector<double> restriction_stage_triggers_owasa = {initial_restriction_triggers[2],
                                                       initial_restriction_triggers[2] + 0.15,
                                                       initial_restriction_triggers[2] + 0.35};
    vector<double> restriction_stage_multipliers_raleigh = {0.9, 0.8, 0.7, 0.6};
    vector<double> restriction_stage_triggers_raleigh = {initial_restriction_triggers[3],
                                                         initial_restriction_triggers[3] + 0.15,
                                                         initial_restriction_triggers[3] + 0.35,
                                                         initial_restriction_triggers[3] + 0.6};

    Restrictions restrictions_c(2,
                                restriction_stage_multipliers_cary,
                                restriction_stage_triggers_cary);
    Restrictions restrictions_d(1,
                                restriction_stage_multipliers_durham,
                                restriction_stage_triggers_durham);
    Restrictions restrictions_o(0,
                                restriction_stage_multipliers_owasa,
                                restriction_stage_triggers_owasa,
                                &owasaDemandClassesFractions,
                                &owasaUserClassesWaterPrices,
                                &owasaPriceSurcharges);
    Restrictions restrictions_r(3,
                                restriction_stage_multipliers_raleigh,
                                restriction_stage_triggers_raleigh);

    drought_mitigation_policies = {&restrictions_c, &restrictions_d,
                                   &restrictions_o, &restrictions_r};

    /// Transfer policy
    /*
     *      2
     *     / \
     *  0 v   v 1
     *   /     \
     *  3---><--1--><--0
     *      2       3
     */

    vector<int> buyers_ids = {0, 1, 3};
    //FIXME: CHECK IF TRANSFER CAPACITIES MATCH IDS IN BUYERS_IDS.
    vector<double> buyers_transfers_capacities = {10.8 * 7, 10.0 * 7, 11.5 * 7,
                                                  7.0 * 7};
    vector<double> buyers_transfers_trigger = {owasa_transfer_trigger,
                                               durham_transfer_trigger,
                                               raleigh_transfer_trigger};

    Graph ug(4);
    ug.addEdge(2,
               1);
    ug.addEdge(2,
               3);
    ug.addEdge(1, 3);
    ug.addEdge(1,
               0);

    Transfers t(4,
                2,
                6,
                35,
                buyers_ids,
                buyers_transfers_capacities,
                buyers_transfers_trigger,
                ug,
                vector<double>(),
                vector<int>());
    drought_mitigation_policies.push_back(&t);


    double insurance_triggers[4] = {owasa_insurance_use,
                                    durham_insurance_use, cary_insurance_use,
                                    raleigh_insurance_use}; //FIXME: Change per solution
    double fixed_payouts[4] = {owasa_insurance_payment,
                               durham_insurance_payment,
                               cary_insurance_payment,
                               raleigh_insurance_payment};
    vector<int> insured_utilities = {0, 1, 2, 3};
    InsuranceStorageToROF in(5,
                             water_sources,
                             g,
                             reservoir_utility_connectivity_matrix,
                             utilities, min_env_flow_controls,
                             insurance_triggers, 1.2, fixed_payouts);

    drought_mitigation_policies.push_back(&in);

    /// Data collector pointer
    MasterDataCollector *data_collector = nullptr;

    /// Creates simulation object
    Simulation s(water_sources,
                 g,
                 reservoir_utility_connectivity_matrix,
                 utilities,
                 drought_mitigation_policies,
                 min_env_flow_controls,
                 n_weeks,
                 max_lines); //2385
    cout << "Beginning simulation." << endl;
//    s.runFullSimulation(n_threads);
    data_collector = s.runFullSimulation(n_threads);
    cout << "Ending simulation" << endl;

    /// Calculate objective values.
    data_collector->setOutputDirectory(output_directory);

    /// Print output files.
    string fu = "/TestFiles/output/Utilities";
    string fws = "/TestFiles/output/WaterSources";
    string fp = "/TestFiles/output/Policies";
    string fo = "/TestFiles/output/Objectives";
    string fpw = "/TestFiles/output/Pathways";

<<<<<<< HEAD
//    data_collector->printUtilitiesOutputCompact(0,
//                                                n_weeks,
//                                                fu + "_s"
//                                                + std::to_string(sol_number));
//    data_collector->printWaterSourcesOutputCompact(0,
//                                                   n_weeks,
//                                                   fws + "_s"
//                                                   + std::to_string(sol_number));
//    data_collector->printPoliciesOutputCompact(0,
//                                               n_weeks,
//                                               fp + "_s"
//                                               + std::to_string(sol_number));
//    data_collector->printUtilitesOutputTabular(0,
//                                               n_weeks,
//                                               fu + "_s"
//                                               + std::to_string(sol_number));
//    data_collector->printWaterSourcesOutputTabular(0,
//                                                   n_weeks,
//                                                   fws + "_s"
//                                                   + std::to_string(sol_number));
    //FIXME:PRINT_POLICIES_OUTPUT_TABULAR BLOWING UP MEMORY.
//    data_collector->printPoliciesOutputTabular(0,
//                                               n_weeks,
//                                               fp + "_s"
//                                               + std::to_string(sol_number));
//    data_collector->printObjectives(fo + "_s" + std::to_string(sol_number));
//    data_collector->printPathways(fpw + "_s" + std::to_string(sol_number));
=======
    data_collector->printUtilitiesOutputCompact(0,
                                                n_weeks,
                                                fu + "_s"
                                                + std::to_string(sol_number));
    data_collector->printWaterSourcesOutputCompact(0,
                                                   n_weeks,
                                                   fws + "_s"
                                                   + std::to_string(sol_number));
    data_collector->printPoliciesOutputCompact(0,
                                               n_weeks,
                                               fp + "_s"
                                               + std::to_string(sol_number));
    data_collector->printUtilitesOutputTabular(0,
                                               n_weeks,
                                               fu + "_s"
                                               + std::to_string(sol_number));
    data_collector->printWaterSourcesOutputTabular(0,
                                                   n_weeks,
                                                   fws + "_s"
                                                   + std::to_string(sol_number));
    data_collector->printPoliciesOutputTabular(0,
                                               n_weeks,
                                               fp + "_s"
                                               + std::to_string(sol_number));
    data_collector->printObjectives(fo + "_s" + std::to_string(sol_number));
    data_collector->printPathways(fpw + "_s" + std::to_string(sol_number));
>>>>>>> parent of a54ecb8... Bug fix: Pathways were not being printed
}

int main(int argc, char *argv[]) {

//    double x_real[57] = {0.963126, //Durham restriction trigger
//                         0.001, //OWASA restriction trigger
//                         0.0165026, //raleigh restriction trigger
//                         0.0158446, //cary restriction trigger
//                         0.00203824, //durham transfer trigger
//                         0.991772, //owasa transfer trigger
//                         0.0759845, //raleigh transfer trigger
//                         0.109251, //OWASA JLA
//                         0.0456465, //Raleigh JLA
//                         0.0503415, //Durham JLA
//                         0.527226, //Cary JLA
//                         0.0768186, //durham annual payment
//                         0.00101558, //owasa annual payment
//                         0.0209892, //raleigh annual payment
//                         0.0997788, //cary annual payment
//                         0.422693, //durham insurance use
//                         0.981162, //owasa insurance use
//                         0.687073, //raleigh insurance use
//                         0.953765, //cary insurance use
//                         0.00151499, //durham insurance payment
//                         0.0189007, //owasa insurance payment
//                         0.0166652, //raleigh insurance payment
//                         0.017396, //cary insurance payment
//                         0.001, //durham inftrigger
//                         0.893212, //owasa inftrigger
//                         0.0114582, //raleigh inftrigger
//                         0.671276, //cary inftrigger
//                         0.61143, //university lake expansion ranking
//                         0.493596, //Cane creek expansion ranking
//                         0.72242, //Quarry (stone?) reservoir expansion (Shallow) ranking
//                         0.874358, //Quarry (stone?) reservoir expansion (deep) ranking
//                         0.938602, //Teer quarry expansion ranking
//                         0.995383, //reclaimed water ranking (low)
//                         0.0152792, //reclaimed water (high)
//                         0.235675, //lake michie expansion ranking (low)
//                         0.993166, //lake michie expansion ranking (high)
//                         0.960062, //little river reservoir ranking
//                         0.467553, //richland creek quarry rank
//                         0.000729602, //neuse river intake rank
//                         1, //reallocate falls lake rank
//                         0.405893, //western wake treatment plant rank OWASA low
//                         0.942959, //western wake treatment plant rank OWASA high
//                         0.00085049, //western wake treatment plant rank durham low
//                         0.0321197, //western wake treatment plant rank durham high
//                         0.000928368, //western wake treatment plant rank raleigh low
//                         0.999997, //western wake treatment plant rank raleigh high
//                         64.0285, //caryupgrades 1
//                         2.0556, //caryugrades 2
//                         5.84962, //caryugrades 3
//                         0.0212207, //western wake treatment plant owasa frac
//                         0.324296, //western wake treatment frac durham
//                         0.169109, //western wake treatment plant raleigh frac
//                         6198.43, //falls lake reallocation
//                         19.7202, //durham inf buffer
//                         19.2861, //owasa inf buffer
//                         17.6756, //raleigh inf buffer
//                         19.7285, //cary inf buffer
//    };
//
//    triangleTest(atoi(argv[1]),
//                 x_real,
//                 atoi(argv[2]),
//                 atoi(argv[3]),
//                 0);


    if (argc == 5)
        for (int i = 0; i < solutions_16.size(); ++i) {
            cout << endl << endl << endl << "Running solution " << i << endl;
            triangleTest(atoi(argv[1]), solutions_16[i].data(), atoi(argv[2]),
                         atoi(argv[3]), i, argv[4]);
        }
    else if (argc == 6) {
        int i = atoi(argv[4]);
        cout << endl << endl << endl << "Running solution " << i << endl;
        triangleTest(atoi(argv[1]), solutions_16[i].data(), atoi(argv[2]),
                     atoi(argv[3]), i, argv[5]);
    }
    else if (argc == 7) {
        int first_sol = atoi(argv[4]);
        int last_sol = atoi(argv[5]);

        cout << "Solutions " << first_sol << " to " << last_sol << endl;

        for (int i = first_sol; i < last_sol; ++i) {
            cout << endl << endl << endl << "Running solution " << i << endl;
            triangleTest(atoi(argv[1]), solutions_16[i].data(), atoi(argv[2]),
                         atoi(argv[3]), i, argv[6]);
        }
    }

    return 0;
}
