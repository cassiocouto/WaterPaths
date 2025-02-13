//
// Created by D.Gold on 8/28/18.
//

#include <algorithm>
#include <numeric>
#include <iostream>
#include <iterator>
#include <fstream>
#include <omp.h>
#include <stdexcept>
#ifdef  PARALLEL
#include <mpi.h>
#endif
#include "PaperTestProblem.h"
#include "../Controls/SeasonalMinEnvFlowControl.h"
#include "../Controls/Custom/FallsLakeMinEnvFlowControl.h"
#include "../Controls/StorageMinEnvFlowControl.h"
#include "../Controls/InflowMinEnvFlowControl.h"
#include "../Controls/FixedMinEnvFlowControl.h"
#include "../Controls/Custom/JordanLakeMinEnvFlowControl.h"
#include "../SystemComponents/WaterSources/AllocatedReservoir.h"
#include "../SystemComponents/WaterSources/Quarry.h"
#include "../SystemComponents/WaterSources/Relocation.h"
#include "../SystemComponents/WaterSources/ReservoirExpansion.h"
#include "../DroughtMitigationInstruments/Transfers.h"
#include "../DroughtMitigationInstruments/InsuranceStorageToROF.h"
#include "../SystemComponents/Bonds/LevelDebtServiceBond.h"
#include "../SystemComponents/Bonds/BalloonPaymentBond.h"
#include "../Simulation/Simulation.h"
#include "../SystemComponents/WaterSources/WaterReuse.h"
#include "../SystemComponents/WaterSources/ReservoirExpansion.h"

#ifdef PARALLEL
void PaperTestProblem::setProblemDefinition(BORG_Problem &problem)
{
    // The parameter bounds are the same for all formulations

    BORG_Problem_set_bounds(problem, 0, 0.001, 1.0);    // watertown restrictions
    BORG_Problem_set_bounds(problem, 1, 0.001, 1.0);    // dryville restrictions
    BORG_Problem_set_bounds(problem, 2, 0.001, 1.0);    // fallsland restrictions
    BORG_Problem_set_bounds(problem, 3, 0.001, 1.0);    // dryville transfer
    BORG_Problem_set_bounds(problem, 4, 0.001, 1.0);    // fallsland transfer
    BORG_Problem_set_bounds(problem, 5, 0.334, 0.9);    // watertown LMA
    BORG_Problem_set_bounds(problem, 6, 0.05, 0.333);   // dryville LMA
    BORG_Problem_set_bounds(problem, 7, 0.05, 0.333);   // fallsland LMA
    BORG_Problem_set_bounds(problem, 8, 0.0, 0.1);      // watertown annual payment
    BORG_Problem_set_bounds(problem, 9, 0.0, 0.1);      // dryville annual payment
    BORG_Problem_set_bounds(problem, 10, 0.0, 0.1);     // fallsland annual payment
    BORG_Problem_set_bounds(problem, 11, 0.001, 1.0);   // watertown insurance use
    BORG_Problem_set_bounds(problem, 12, 0.001, 1.0);   // dryville insurance use
    BORG_Problem_set_bounds(problem, 13, 0.001, 1.0);   // fallsland insurance use
    BORG_Problem_set_bounds(problem, 14, 0.0, 0.02);    // watertown insurance payment
    BORG_Problem_set_bounds(problem, 15, 0.0, 0.02);    // dryville insurance payment
    BORG_Problem_set_bounds(problem, 16, 0.0, 0.02);    // fallsland insurance payment
    BORG_Problem_set_bounds(problem, 17, 0.001, 1.0);   // watertown inf trigger
    BORG_Problem_set_bounds(problem, 18, 0.001, 1.0);   // dryville inf trigger
    BORG_Problem_set_bounds(problem, 19, 0.001, 1.0);   // fallsland inf trigger
    BORG_Problem_set_bounds(problem, 20, 0.0, 1.0);     // new river rank watertown
    BORG_Problem_set_bounds(problem, 21, 0.0, 1.0);     // college rock expansion rank low
    BORG_Problem_set_bounds(problem, 22, 0.0, 1.0);     // college rock expansion rank high
    BORG_Problem_set_bounds(problem, 23, 0.0, 1.0);     // watertown reuse rank
    BORG_Problem_set_bounds(problem, 24, 0.0, 1.0);     // sugar creek rank
    BORG_Problem_set_bounds(problem, 25, 0.0, 1.0);     // granite quarry rank
    BORG_Problem_set_bounds(problem, 26, 0.0, 1.0);     // new river rank fallsland


    // Set epsilons for objectives

    BORG_Problem_set_epsilon(problem, 0, 0.001);
    BORG_Problem_set_epsilon(problem, 1, 0.02);
    BORG_Problem_set_epsilon(problem, 2, 25.0);
    BORG_Problem_set_epsilon(problem, 3, 0.02);
    BORG_Problem_set_epsilon(problem, 4, 0.05);
}
#endif

/**
 * Runs three utility carolina problem for demonstration paper
 * @param vars
 * @param n_realizations
 * @param n_weeks
 * @param sol_number
 * @param output_directory
 * @todo test and edit
 */

int PaperTestProblem::functionEvaluation(double *vars, double *objs, double *consts) {

    // ===================== SET UP DECISION VARIABLES  =====================

    //FIXME why do we make a null pointer here?
    Simulation *s = nullptr;
    double calibrate_volume_multiplier = 1.5;

    double watertown_restriction_trigger = vars[0];
    double dryville_restriction_trigger = vars[1];
    double fallsland_restriction_trigger = vars[2];
    double dryville_transfer_trigger = vars[3];
    double fallsland_transfer_trigger = vars[4];
    double watertown_LMA = vars[5];
    double dryville_LMA = vars[6];
    double fallsland_LMA = vars[7];
    double watertown_annual_payment = vars[8];
    double dryville_annual_payment = vars[9];
    double fallsland_annual_payment = vars[10];
    double watertown_insurance_use = vars[11];
    double dryville_insurance_use = vars[12];
    double fallsland_insurance_use = vars[13];
    double watertown_insurance_payment = vars[14];
    double dryville_insurance_payment = vars[15];
    double fallsland_insurance_payment = vars[16];

    double watertown_inftrigger = vars[17];
    //double watertown_inftrigger = 1.1;
    double dryville_inftrigger = vars[18];
    //double dryville_inftrigger = 1.1;
    double fallsland_inftrigger = vars[19];
    //double fallsland_inftrigger = 1.1;

    if (import_export_rof_tables == EXPORT_ROF_TABLES) {
        dryville_inftrigger = 1.1;
        fallsland_inftrigger = 1.1;
        watertown_inftrigger = 1.1;
    };

    double new_river_rank_watertown = vars[20];
    //double new_river_rank_watertown = .1;
    double college_rock_expansion_low_rank = vars[21];
    //double college_rock_expansion_low_rank = .2;
    double college_rock_expansion_high_rank = vars[22];
    //double college_rock_expansion_high_rank = .03;
    double watertown_reuse_rank = vars[23];
    //double watertown_reuse_rank = .4;
    double sugar_creek_rank =vars[24];
    //double sugar_creek_rank = .1;
    double granite_quarry_rank = vars[25];
    //double granite_quarry_rank = .2;
    double new_river_rank_fallsland = vars[26];
    //double new_river_rank_fallsland = .1;

    vector<infraRank> dryville_infra_order_raw = {
            infraRank(3, sugar_creek_rank),
            infraRank(5, granite_quarry_rank)
    };

    vector<infraRank> fallsland_infra_order_raw = {
            infraRank(4, new_river_rank_fallsland)
    };

    vector<infraRank> watertown_infra_order_raw = {
            infraRank(4, new_river_rank_watertown),
            infraRank(7, college_rock_expansion_low_rank),
            infraRank(8, college_rock_expansion_high_rank),
            infraRank(9, watertown_reuse_rank)
    };

    sort(dryville_infra_order_raw.begin(),
         dryville_infra_order_raw.end(),
         by_xreal());
    // commenting out because vector only has length 1
    //sort(fallsland_infra_order_raw.begin(),
    //    fallsland_infra_order_raw.end(),
    //    by_xreal());
    sort(watertown_infra_order_raw.begin(),
         watertown_infra_order_raw.end(),
         by_xreal());

    vector<int> rof_triggered_infra_order_dryville =
            vecInfraRankToVecInt(dryville_infra_order_raw);
    vector<double> rofs_infra_dryville = vector<double>
            (rof_triggered_infra_order_dryville.size(), dryville_inftrigger);

    vector<int> rof_triggered_infra_order_fallsland =
            vecInfraRankToVecInt(fallsland_infra_order_raw);
    vector<double> rofs_infra_fallsland = vector<double>
            (rof_triggered_infra_order_fallsland.size(), fallsland_inftrigger);

    vector<int> rof_triggered_infra_order_watertown =
            vecInfraRankToVecInt(watertown_infra_order_raw);
    vector<double> rofs_infra_watertown = vector<double>
            (rof_triggered_infra_order_watertown.size(), watertown_inftrigger);


    /// Remove small expansions being built after big expansions that would
    /// encompass the smal expansions.
    double added_storage_college_rock_expansion_low = 500;
    double added_storage_college_rock_expansion_high = 1000;

    checkAndFixInfraExpansionHighLowOrder(
            &rof_triggered_infra_order_watertown, &rofs_infra_watertown,
            7,
            8,
            added_storage_college_rock_expansion_low,
            added_storage_college_rock_expansion_high);

    double watertown_demand_buffer = 1.0;
    double dryville_demand_buffer = 1.0;
    double fallsland_demand_buffer = 1.0;

    // Normalize lake michael allocations in case they exceed 1
    double sum_lma_allocations = dryville_LMA + fallsland_LMA + watertown_LMA;
    if (sum_lma_allocations == 0.)
        throw invalid_argument("LMA allocations cannot be all "
                                 "zero.");
    if (sum_lma_allocations > 1){
        dryville_LMA /= sum_lma_allocations;
        fallsland_LMA /= sum_lma_allocations;
        watertown_LMA /= sum_lma_allocations;
    }


    // ==================== SET UP RDM FACTORS ============================

    if (utilities_rdm.empty()) {
        /// All matrices below have dimensions n_realizations x nr_rdm_factors
        utilities_rdm = std::vector<vector<double>>(
                n_realizations, vector<double>(4, 1.));
        water_sources_rdm = std::vector<vector<double>>(
                n_realizations, vector<double>(51, 1.));
        policies_rdm = std::vector<vector<double>>(
                n_realizations, vector<double>(4, 1.));
    }


    // ===================== SET UP PROBLEM COMPONENTS =====================
//Beginning with Reservoir continuity

    int streamflow_n_weeks = 52 * (70 + 50);

    EvaporationSeries evaporation_durham(&evap_durham, streamflow_n_weeks); //Evaporation
    EvaporationSeries evaporation_falls_lake(&evap_falls_lake, streamflow_n_weeks); //Evaporation
    EvaporationSeries evaporation_jordan_lake(&evap_jordan_lake, streamflow_n_weeks); // Lake Michael
    EvaporationSeries evaporation_owasa(&evap_owasa, streamflow_n_weeks);

    // Create catchments and corresponding vectors

    // Autumn Lake (abstracted Neuse River Basin)
    Catchment durham_inflows(&streamflows_durham, streamflow_n_weeks); // use Durham inflows for half the inflows
    Catchment lower_flat_river(&streamflows_flat, streamflow_n_weeks);
    Catchment swift_creek(&streamflows_swift, streamflow_n_weeks);


    // Add catchments to vector
    vector<Catchment *> catchment_autumn;
    catchment_autumn.push_back(&durham_inflows);
    catchment_autumn.push_back(&lower_flat_river);
    //catchment_autumn.push_back(&little_river_raleigh);
    catchment_autumn.push_back(&swift_creek);

    // College Rock Reservoir Catchment (abstracted from upper cape fear)
    Catchment phils_creek(&streamflows_phils, streamflow_n_weeks);
    Catchment morgan_creek(&streamflows_morgan, streamflow_n_weeks);

    // Add catchments to vector
    // College Rock (University Lake)
    vector<Catchment *> catchment_college_rock;
    catchment_college_rock.push_back(&morgan_creek);

    // Granite Quarry (Stone Quarry)
    vector<Catchment *> catchment_granite_quarry;
    catchment_granite_quarry.push_back(&phils_creek);

    // Sugar Creek (Cane Creek)
    Catchment sugar_creek(&streamflows_cane, streamflow_n_weeks);

    // Add catchment to vector
    vector<Catchment *> catchment_sugar_creek;
    catchment_sugar_creek.push_back(&sugar_creek);

    // Lake Michael Catchment (abstracted lower cape fear)
    Catchment lower_haw_river(&streamflows_haw, streamflow_n_weeks);

    // Add catchment to vector
    vector<Catchment *> catchment_lower_haw_river;
    catchment_lower_haw_river.push_back(&lower_flat_river);

    // New River Reservoir catchment
    Catchment little_river_raleigh(&streamflows_llr, streamflow_n_weeks);

    vector<Catchment *> catchment_new_river;
    catchment_new_river.push_back(&little_river_raleigh);

    // Downstream Gage
    Catchment cape_fear_river_at_lillington(&streamflows_lillington, streamflow_n_weeks);

    // Add catchment to vector
    vector<Catchment *> gage_lillington;
    gage_lillington.push_back(&cape_fear_river_at_lillington);

    // Create storage vs. area reservoir curves
    //FIXME DISCUSS WITH BERNARDO HOW THIS WORKS TO MAKE SURE CORRECT, WHY DO ONLY SOME RES HAVE THESE CURVES?

    // CURRENTLY THIS IS THE STORAGE OF FALLS + DURHAM (MICHIE AND LR) + WB
    // ASSUMING DURHAM AND WB SCALE PROPORTIONALLY TO FALLS
    // WB and Durham storage added to water supply capacity
    double autumn_lake_supply_capacity = (14700.0 + 6349 + 2790 + 6000) * table_gen_storage_multiplier * calibrate_volume_multiplier; //6000 added to balance test problem.
    double autumn_lake_wq_capacity = 20000.0 * table_gen_storage_multiplier * calibrate_volume_multiplier;
    double autumn_lake_storage_capacity = autumn_lake_wq_capacity + autumn_lake_supply_capacity;
    vector<double> autumn_lake_storage = {0, autumn_lake_supply_capacity,
                                          autumn_lake_storage_capacity};
    vector<double> autumn_lake_area = {0.32 * 5734, 0.32 * 29000, 0.28 * 40434};
    DataSeries autumn_lake_storage_area(&autumn_lake_storage, &autumn_lake_area);

    vector<double> new_river_res_storage = {0, 1700};
    vector<double> new_river_res_area = {0, 0.3675 * new_river_res_storage[1]};
    DataSeries new_river_storage_area(&new_river_res_storage,
                                      &new_river_res_area);

    vector<double> sugar_creek_res_storage = {0, 1500};//2909};
    vector<double> sugar_creek_res_area = {0, 0.3675 * 2909};
    DataSeries sugar_creek_storage_area(&sugar_creek_res_storage,
                                        &sugar_creek_res_area);

    vector<double> granite_quarry_storage = {0, 200};
    vector<double> granite_quarry_area = {0, 0.3675 * 200};
    DataSeries granite_quarry_storage_area(&granite_quarry_storage,
                                           &granite_quarry_area);

    // Create minimum environmental flow rules (controls)
    // Autumn is combining Falls+Durham+WB
    //FIXME FIX THESE FLOW REQUIREMENTS
    vector<int> autumn_controls_weeks = {0, 13, 43, 53};
    vector<double> autumn_releases = {(39 + 10 + 2) * 7, (65 + 4 + 1) * 7, (39 + 10 + 2) * 7};

    SeasonalMinEnvFlowControl autumn_min_env_control(2, autumn_controls_weeks, autumn_releases);

    // Lake Michael is based off the Jordan Lake and uses its class
    JordanLakeMinEnvFlowControl lake_michael_min_env_control( 1,
                                                              cape_fear_river_at_lillington, 64.63, 129.26, 25.85, 193.89,
                                                              290.84, 387.79, 30825.0 * table_gen_storage_multiplier * calibrate_volume_multiplier, 10300.0 * table_gen_storage_multiplier * calibrate_volume_multiplier);

    //FIXME SUGAR CREEK BASED ON CCR, SO LEAVING AS IS, IS THIS A GOOD IDEA?
    vector<double> sugar_creek_inflows = {0.1422 * 7, 0.5 * 7, 1 * 7, 1.5 * 7,
                                          1.797 * 7};
    vector<double> sugar_creek_releases = {0.1422 * 7, 0.5 * 7, 1 * 7, 1.5 * 7,
                                           1.797 * 7};

    InflowMinEnvFlowControl sugar_creek_min_env_control(3,
                                                        sugar_creek_inflows,
                                                        sugar_creek_releases);

    // College Rock and Granite Quarry have no min flow
    FixedMinEnvFlowControl college_rock_min_env_control(0, 0);
    FixedMinEnvFlowControl granite_quarry_min_env_control(0, 0);

    //FIXME made these numbers up
    vector<int> new_river_controls_weeks = {0, 13, 43, 53};
    vector<double> new_river_releases = {3 * 7, 8 * 7, 3 * 7};

    SeasonalMinEnvFlowControl new_river_min_env_control(4, new_river_controls_weeks, new_river_releases);

    vector<MinEnvFlowControl *> min_env_flow_controls;
    min_env_flow_controls.push_back(&autumn_min_env_control);
    min_env_flow_controls.push_back(&lake_michael_min_env_control);
    min_env_flow_controls.push_back(&sugar_creek_min_env_control);
    min_env_flow_controls.push_back(&college_rock_min_env_control);
    min_env_flow_controls.push_back(&granite_quarry_min_env_control);


    // Lake Michael parameters
    double lake_michael_supply_capacity = 8100 * table_gen_storage_multiplier * calibrate_volume_multiplier; // reduced to .69 of JL cap
    double lake_michael_wq_capacity = 30825.0 * table_gen_storage_multiplier * calibrate_volume_multiplier;
    double lake_michael_storage_capacity = lake_michael_wq_capacity + lake_michael_supply_capacity;
    vector<int> lake_michael_allocations_ids = {0, 1, 2, WATER_QUALITY_ALLOCATION};
    vector<double> lake_michael_allocation_fractions = {
            watertown_LMA * lake_michael_supply_capacity / lake_michael_storage_capacity,
            dryville_LMA * lake_michael_supply_capacity / lake_michael_storage_capacity,
            fallsland_LMA * lake_michael_supply_capacity / lake_michael_storage_capacity,
            lake_michael_wq_capacity / lake_michael_storage_capacity};
    vector<double> lake_michael_treatment_allocation_fractions = {1., 0.0, 0.0};

    // Autumn Lake parameters
    //FIXME: can reallocate to make more interesting
    vector<int> autumn_lake_allocations_ids = {1, 2, WATER_QUALITY_ALLOCATION};
    vector<double> autumn_lake_allocation_fractions = {
            0.29 * autumn_lake_supply_capacity / autumn_lake_storage_capacity,
            0.71 * autumn_lake_supply_capacity / autumn_lake_storage_capacity,
            autumn_lake_wq_capacity / autumn_lake_storage_capacity};
    vector<double> autumn_lake_treatment_allocation_fractions = {0.38, 0.62};

// Create existing reservoirs
    /// combined university lake and stone quarry
    Reservoir college_rock_reservoir("College Rock Reservoir",
                                     0,
                                     catchment_college_rock,
                                     449 * table_gen_storage_multiplier * calibrate_volume_multiplier,
                                     ILLIMITED_TREATMENT_CAPACITY,
                                     evaporation_owasa,
                                     222);

    AllocatedReservoir lake_michael("Lake Michael",
                                    1,
                                    catchment_lower_haw_river,
                                    lake_michael_storage_capacity,
                                    448,
                                    evaporation_jordan_lake,
                                    13940,
                                    &lake_michael_allocations_ids,
                                    &lake_michael_allocation_fractions,
                                    &lake_michael_treatment_allocation_fractions);

    AllocatedReservoir autumn_lake("Autumn Lake",
                                   2,
                                   catchment_autumn,
                                   autumn_lake_storage_capacity,
                                   ILLIMITED_TREATMENT_CAPACITY,
                                   evaporation_falls_lake,
                                   &autumn_lake_storage_area,
                                   &autumn_lake_allocations_ids,
                                   &autumn_lake_allocation_fractions,
                                   &autumn_lake_treatment_allocation_fractions);

    // Create potential sources

    vector<double> city_infrastructure_rof_triggers = {watertown_inftrigger,
                                                       dryville_inftrigger,
                                                       fallsland_inftrigger};

    //FIXME ORIGINAL CODE SETS WEEKS_IN_YEAR TO 0 HERE
    vector<double> construction_time_interval = {3.0, 5.0};

    LevelDebtServiceBond sugar_bond(5, 150.0, 25, 0.05, vector<int>(1, 0));
    Reservoir sugar_creek_reservoir("Sugar Creek Reservoir",
                                    5,
                                    catchment_sugar_creek,
                                    1500, //2909,  VALUE CHANGED FOR CALIBRATION OF TEST PROBLEM.
                                    ILLIMITED_TREATMENT_CAPACITY,
                                    evaporation_owasa,
                                    &sugar_creek_storage_area,
                                    construction_time_interval,
                                    17 * WEEKS_IN_YEAR,
                                    sugar_bond);

    BalloonPaymentBond granite_bond(6, 22.6, 25, 0.05, vector<int>(1, 0), 3);
    Reservoir granite_quarry("Granite Quarry",
                             6,
                             catchment_granite_quarry,
                             200,
                             ILLIMITED_TREATMENT_CAPACITY,
                             evaporation_owasa,
                             &granite_quarry_storage_area,
                             construction_time_interval,
                             17 * WEEKS_IN_YEAR,
                             granite_bond);

    //FIXME check bond, this one is from little river raliegh
    vector<int> nrr_allocations_ids = {1, 2, WATER_QUALITY_ALLOCATION};
    vector<double> nrr_allocation_fractions = {0.5, 0.2, 0.3};
    vector<double> nrr_treatment_allocation_fractions = {0.5, 0.5};
    LevelDebtServiceBond new_river_bond(4, 263.0, 25, 0.05, vector<int>(1, 0));
    AllocatedReservoir new_river_reservoir("New River Reservoir",
                                           4,
                                           catchment_new_river,
                                           new_river_res_storage[1],
                                           ILLIMITED_TREATMENT_CAPACITY,
                                           evaporation_falls_lake,
                                           &new_river_storage_area,
                                           construction_time_interval,
                                           17 * WEEKS_IN_YEAR,
                                           new_river_bond,
                                           &nrr_allocations_ids,
                                           &nrr_allocation_fractions,
                                           &nrr_treatment_allocation_fractions);

    LevelDebtServiceBond dummy_bond(3, 1., 1, 1., vector<int>(1, 0));
    Reservoir dummy_endpoint("Dummy Node", 3, vector<Catchment *>(), 1., 0, evaporation_durham, 1,
                             construction_time_interval, 0, dummy_bond);

    //FIXME: Edit the expansion volumes for CRR, just made these up
    //FIXME: changed these temporarily to 0
    vector<double> college_rock_expansion_low_construction_time = {3, 5};
    LevelDebtServiceBond college_rock_expansion_low_bond(7, 50, 30, .05, vector<int>(1, 0));
    ReservoirExpansion college_rock_expansion_low((char *) "College Rock Expansion Low", 7, 0, 500,
                                                  college_rock_expansion_low_construction_time, 5, college_rock_expansion_low_bond);

    vector<double> college_rock_expansion_high_construction_time = {3, 5};
    LevelDebtServiceBond college_rock_expansion_high_bond(8, 100, 30, .05, vector<int>(1, 0));
    ReservoirExpansion college_rock_expansion_high((char *) "College Rock Expansion High", 8, 0, 1000,
                                                   college_rock_expansion_high_construction_time, 5,
                                                   college_rock_expansion_high_bond);

    vector<double> watertown_reuse_construction_time = {3, 5};
    LevelDebtServiceBond watertown_reuse_bond(9, 50, 30, .05, vector<int>(1, 0));
    WaterReuse watertown_reuse((char *) "Watertown Reuse", 9, 20, watertown_reuse_construction_time, 5,
                               watertown_reuse_bond);

    vector<WaterSource *> water_sources;
    water_sources.push_back(&college_rock_reservoir);
    water_sources.push_back(&autumn_lake);
    water_sources.push_back(&lake_michael);
    water_sources.push_back(&sugar_creek_reservoir);
    water_sources.push_back(&new_river_reservoir);
    water_sources.push_back(&granite_quarry);
    water_sources.push_back(&college_rock_expansion_low);
    water_sources.push_back(&college_rock_expansion_high);
    water_sources.push_back(&watertown_reuse);
    water_sources.push_back(&dummy_endpoint);

/*
 *
 *  0 College Rock Reservoir (7) small expansion (8) large expansion
 *   \
 *    \                          (6) Granite Quarry
 *     \                         /
 *      \                      (5) Sugar Creek Reservoir
 *       1 Lake Michael        /
 *        \                   /
 *         \                 2 Autumn Lake
 *          \               /
 *    Lillington           /
 *            \           /
 *             \         /
 *              \       /
 *               \    (4) New River Reservoir
 *                \   /
 *                 \ /    (9) watertown reuse
 *                  |
 *                  |
 *                  3 Dummy Endpoint
 */

    Graph g(7);
    g.addEdge(0, 1);
    g.addEdge(1, 3);
    g.addEdge(6, 5);
    g.addEdge(5, 2);
    g.addEdge(2, 4);
    g.addEdge(4, 3);

    auto demand_n_weeks = (int) round(46 * WEEKS_IN_YEAR);

    vector<double> bond_term = {25, 25, 25, 25};
    vector<double> bond_rate = {0.05, 0.05, 0.05, 0.05};
    double discount_rate = 0.05;

    //FIXME make return flows after utilities are created?
    vector<int> watertown_ws_return_id;
    vector<vector<double>> watertown_discharge_fraction_series;
    WwtpDischargeRule wwtp_discharge_watertown(
            watertown_discharge_fraction_series,
            watertown_ws_return_id);

    vector<int> dryville_ws_return_id = {4};
    vector<vector<double>> dryville_discharge_fraction_series;
    WwtpDischargeRule wwtp_discharge_dryville(
            demand_to_wastewater_fraction_dryville,
            dryville_ws_return_id);

    vector<int> fallsland_ws_return_id = {4};
    vector<vector<double>> fallsland_discharge_fraction_series;
    WwtpDischargeRule wwtp_discharge_fallsland(
            demand_to_wastewater_fraction_fallsland,
            fallsland_ws_return_id);

    //FIXME: bond etc need to be updated, should chat about demand buffer
    Utility watertown((char *) "Watertown", 0, demand_watertown, demand_n_weeks, watertown_annual_payment,
                      &watertownDemandClassesFractions,
                      &watertownUserClassesWaterPrices, wwtp_discharge_watertown, watertown_demand_buffer,
                      rof_triggered_infra_order_watertown, vector<int>(), rofs_infra_watertown, discount_rate, 30, .05);

    //FIXME: bond etc need to be updated, should chat about demand buffer
    Utility dryville((char *) "Dryville", 1, demand_dryville, demand_n_weeks, dryville_annual_payment,
                     &dryvilleDemandClassesFractions, &dryvilleUserClassesWaterPrices,
                     wwtp_discharge_dryville, dryville_demand_buffer, rof_triggered_infra_order_dryville, vector<int>(),
                     rofs_infra_dryville, discount_rate, 30, 0.05);

    Utility fallsland((char *) "Fallsland", 2, demand_fallsland, demand_n_weeks, fallsland_annual_payment,
                      &fallslandDemandClassesFractions, &fallslandUserClassesWaterPrices,
                      wwtp_discharge_fallsland, fallsland_demand_buffer, rof_triggered_infra_order_fallsland,
                      vector<int>(), rofs_infra_fallsland, discount_rate, 30, 0.05);

    vector<Utility*> utilities;
    utilities.push_back(&watertown);
    utilities.push_back(&dryville);
    utilities.push_back(&fallsland);

    vector<vector<int>> reservoir_utility_connectivity_matrix = {
            {0, 1, 4, 7, 8, 9}, //Watertown
            {2, 5, 3}, //Dryville
            {2, 4} //Fallsland
    };

    auto table_storage_shift = vector<vector<double>>(3, vector<double>(water_sources.size() + 1, 0.));
    table_storage_shift[2][4] = 1500;
    table_storage_shift[1][5] = 100;

    vector<DroughtMitigationPolicy *> drought_mitigation_policies;
    vector<double> initial_restriction_triggers = {watertown_restriction_trigger,
                                                   dryville_restriction_trigger,
                                                   fallsland_restriction_trigger};

    vector<double> restriction_stage_multipliers_watertown = {0.9, 0.8, 0.7, 0.6};
    vector<double> restriction_stage_triggers_watertown = {initial_restriction_triggers[0],
                                                           initial_restriction_triggers[0] + 0.15f,
                                                           initial_restriction_triggers[0] + 0.35f,
                                                           initial_restriction_triggers[0] + 0.6f};

    vector<double> restriction_stage_multipliers_dryville = {0.9, 0.8, 0.7, 0.6};
    vector<double> restriction_stage_triggers_dryville = {initial_restriction_triggers[1],
                                                          initial_restriction_triggers[1] + 0.15f,
                                                          initial_restriction_triggers[1] + 0.35f,
                                                          initial_restriction_triggers[1] + 0.6f};

    vector<double> restriction_stage_multipliers_fallsland = {0.9, 0.8, 0.7, 0.6};
    vector<double> restriction_stage_triggers_fallsland = {initial_restriction_triggers[2],
                                                           initial_restriction_triggers[2] + 0.15f,
                                                           initial_restriction_triggers[2] + 0.35f,
                                                           initial_restriction_triggers[2] + 0.6f};

    Restrictions restrictions_w(0,
                                restriction_stage_multipliers_watertown,
                                restriction_stage_triggers_watertown);

    Restrictions restrictions_d(1,
                                restriction_stage_multipliers_dryville,
                                restriction_stage_triggers_dryville);

    Restrictions restrictions_f(2,
                                restriction_stage_multipliers_fallsland,
                                restriction_stage_triggers_fallsland);

    drought_mitigation_policies = {&restrictions_w, &restrictions_d, &restrictions_f};


    // Transfer policy

    /*
     *      0
     *     / \
     *  0 v   v 1
     *   /     \
     *  1--> <--2
     *      2
     */

    vector<int> buyers_ids = {1, 2};

    //FIXME: TEST VALUES, MAY WANT TO EDIT
    vector<double> buyer_transfer_capacities = {10.0*7, 10.0*7, 10.0*7};

    vector<double> buyer_transfer_triggers = {dryville_transfer_trigger,
                                              fallsland_transfer_trigger};

    Graph ug(3);
    ug.addEdge(0,1);
    ug.addEdge(0,2);
    ug.addEdge(1,2);

    Transfers t(3, 0, 1, 35,
                buyers_ids,
                buyer_transfer_capacities,
                buyer_transfer_triggers,
                ug,
                vector<double>(),
                vector<int>());
    drought_mitigation_policies.push_back(&t);


    /// Set up insurance
    vector<double> insurance_triggers = {watertown_insurance_use,
                                         dryville_insurance_use,
                                         fallsland_insurance_use};
    vector<double> fixed_payouts = {watertown_insurance_payment,
                                    dryville_insurance_payment,
                                    fallsland_insurance_payment};

    vector<int> insured_utilities = {0, 1, 2};

    InsuranceStorageToROF in(4, water_sources, g, reservoir_utility_connectivity_matrix, utilities,
                             min_env_flow_controls, utilities_rdm, water_sources_rdm, insurance_triggers, 1.2,
                             fixed_payouts, n_weeks);

    drought_mitigation_policies.push_back(&in);


    /// Creates simulation object depending on use (or lack thereof) ROF tables
    double start_time = omp_get_wtime();
    if (import_export_rof_tables == EXPORT_ROF_TABLES) {
        s = new Simulation(water_sources,
                           g,
                           reservoir_utility_connectivity_matrix,
                           utilities,
                           drought_mitigation_policies,
                           min_env_flow_controls,
                           utilities_rdm,
                           water_sources_rdm,
                           policies_rdm,
                           n_weeks,
                           realizations_to_run,
                           rof_tables_directory);
        this->master_data_collector = s->runFullSimulation(n_threads);
    } else if (import_export_rof_tables == IMPORT_ROF_TABLES) {
        s = new Simulation (water_sources,
                            g,
                            reservoir_utility_connectivity_matrix,
                            utilities,
                            drought_mitigation_policies,
                            min_env_flow_controls,
                            utilities_rdm,
                            water_sources_rdm,
                            policies_rdm,
                            n_weeks,
                            realizations_to_run,
                            rof_tables,
                            table_storage_shift,
                            rof_tables_directory);
        this->master_data_collector = s->runFullSimulation(n_threads);
    } else {
        s = new Simulation(water_sources,
                           g,
                           reservoir_utility_connectivity_matrix,
                           utilities,
                           drought_mitigation_policies,
                           min_env_flow_controls,
                           utilities_rdm,
                           water_sources_rdm,
                           policies_rdm,
                           n_weeks,
                           realizations_to_run);
        this->master_data_collector = s->runFullSimulation(n_threads);
    }
    double end_time = omp_get_wtime();
    printf("Function evaluation time: %f\n", end_time - start_time);

    /// Calculate objectives and store them in Borg decision variables array.
#ifdef  PARALLEL
    objectives = calculateAndPrintObjectives(false);

        int i = 0;
        objs[i] = 1. - min(min(objectives[i], objectives[5 + i]),
        		   objectives[10 + i]);
        for (i = 1; i < 5; ++i) {
            objs[i] = max(max(objectives[i], objectives[5 + i]),
      	                  objectives[10 + i]);
        }

        if (s != nullptr) {
            delete s;
	}
	s = nullptr;
#endif
//    } catch (const std::exception& e) {
//        simulationExceptionHander(e, s, objs, vars);
//	return 1;
//    }

    delete s;

    return 0;

};

PaperTestProblem::PaperTestProblem(unsigned long n_weeks, int import_export_rof_table)
        : Problem(n_weeks) {
    if (import_export_rof_table == EXPORT_ROF_TABLES) {
        table_gen_storage_multiplier = BASE_STORAGE_CAPACITY_MULTIPLIER;
    } else {
        table_gen_storage_multiplier = 1.;
    }
}


PaperTestProblem::~PaperTestProblem() = default;


void PaperTestProblem::readInputData() {
    cout << "Reading input data." << endl;
    string data_dir = DEFAULT_DATA_DIR + BAR;

#pragma omp parallel num_threads(omp_get_thread_num())
    {
#pragma omp single
        streamflows_durham = Utils::parse2DCsvFile(
                io_directory + DEFAULT_DATA_DIR + "inflows" + evap_inflows_suffix +
                BAR + "durham_inflows.csv", n_realizations);
#pragma omp single
        streamflows_flat = Utils::parse2DCsvFile(
                io_directory + DEFAULT_DATA_DIR + "inflows" + evap_inflows_suffix +
                BAR + "falls_lake_inflows.csv", n_realizations);
#pragma omp single
        streamflows_swift = Utils::parse2DCsvFile(
                io_directory + DEFAULT_DATA_DIR + "inflows" + evap_inflows_suffix +
                BAR + "lake_wb_inflows.csv", n_realizations);
#pragma omp single
        streamflows_llr = Utils::parse2DCsvFile(
                io_directory + DEFAULT_DATA_DIR + "inflows" + evap_inflows_suffix +
                BAR + "little_river_raleigh_inflows.csv", n_realizations);
#pragma omp single
        streamflows_phils = Utils::parse2DCsvFile(
                io_directory + DEFAULT_DATA_DIR + "inflows" + evap_inflows_suffix +
                BAR + "stone_quarry_inflows.csv", n_realizations);
#pragma omp single
        streamflows_cane = Utils::parse2DCsvFile(
                io_directory + DEFAULT_DATA_DIR + "inflows" + evap_inflows_suffix +
                BAR + "cane_creek_inflows.csv", n_realizations);
#pragma omp single
        streamflows_morgan = Utils::parse2DCsvFile(
                io_directory + DEFAULT_DATA_DIR + "inflows" + evap_inflows_suffix +
                BAR + "university_lake_inflows.csv", n_realizations);
#pragma omp single
        streamflows_haw = Utils::parse2DCsvFile(
                io_directory + DEFAULT_DATA_DIR + "inflows" + evap_inflows_suffix +
                BAR + "jordan_lake_inflows.csv", n_realizations);
#pragma omp single
        streamflows_lillington = Utils::parse2DCsvFile(
                io_directory + DEFAULT_DATA_DIR + "inflows" + evap_inflows_suffix +
                BAR + "lillington_inflows.csv", n_realizations);
// };
        //cout << "Reading evaporations." << endl;
#pragma omp single
        evap_durham = Utils::parse2DCsvFile(
                io_directory + DEFAULT_DATA_DIR + "evaporation" + evap_inflows_suffix +
                BAR + "durham_evap.csv", n_realizations);
#pragma omp single
        evap_falls_lake = Utils::parse2DCsvFile(
                io_directory + DEFAULT_DATA_DIR + "evaporation" + evap_inflows_suffix +
                BAR + "falls_lake_evap.csv", n_realizations);
#pragma omp single
        evap_owasa = Utils::parse2DCsvFile(
                io_directory + DEFAULT_DATA_DIR + "evaporation" + evap_inflows_suffix +
                BAR + "owasa_evap.csv", n_realizations);
#pragma omp single
        evap_little_river = Utils::parse2DCsvFile(
                io_directory + DEFAULT_DATA_DIR + "evaporation" + evap_inflows_suffix +
                BAR + "little_river_raleigh_evap.csv", n_realizations);
#pragma omp single
        {
            evap_wheeler_benson = Utils::parse2DCsvFile(
                    io_directory + DEFAULT_DATA_DIR + "evaporation" + evap_inflows_suffix +
                    BAR + "wb_evap.csv", n_realizations);
            evap_jordan_lake = evap_owasa;
        }

        //cout << "Reading demands." << endl;
#pragma omp single
        demand_watertown = Utils::parse2DCsvFile(
                io_directory + DEFAULT_DATA_DIR + "demands" + evap_inflows_suffix +
                BAR + "cary_demand.csv", n_realizations);
#pragma omp single
        demand_dryville = Utils::parse2DCsvFile(
                io_directory + DEFAULT_DATA_DIR + "demands" + evap_inflows_suffix +
                BAR + "durham_demand.csv", n_realizations);
#pragma omp single
        demand_fallsland = Utils::parse2DCsvFile(
                io_directory + DEFAULT_DATA_DIR + "demands" + evap_inflows_suffix +
                BAR + "raleigh_demand.csv", n_realizations);

        //cout << "Reading others." << endl;
#pragma omp single
        {
            demand_to_wastewater_fraction_fallsland = Utils::parse2DCsvFile(
                    io_directory + DEFAULT_DATA_DIR + "demand_to_wastewater_fraction_owasa_raleigh.csv");
            demand_to_wastewater_fraction_dryville = Utils::parse2DCsvFile(
                    io_directory + DEFAULT_DATA_DIR + "demand_to_wastewater_fraction_owasa_raleigh.csv");

            watertownDemandClassesFractions = Utils::parse2DCsvFile(
                    io_directory + DEFAULT_DATA_DIR + "caryDemandClassesFractions.csv");
            dryvilleDemandClassesFractions = Utils::parse2DCsvFile(
                    io_directory + DEFAULT_DATA_DIR + "durhamDemandClassesFractions.csv");
            fallslandDemandClassesFractions = Utils::parse2DCsvFile(
                    io_directory + DEFAULT_DATA_DIR + "raleighDemandClassesFractions.csv");

            watertownUserClassesWaterPrices = Utils::parse2DCsvFile(
                    io_directory + DEFAULT_DATA_DIR + "caryUserClassesWaterPrices.csv");
            dryvilleUserClassesWaterPrices = Utils::parse2DCsvFile(
                    io_directory + DEFAULT_DATA_DIR + "durhamUserClassesWaterPrices.csv");
            fallslandUserClassesWaterPrices = Utils::parse2DCsvFile(
                    io_directory + DEFAULT_DATA_DIR + "raleighUserClassesWaterPrices.csv");
        }
//    cout << "Done reading input data." << endl;
    }

}


