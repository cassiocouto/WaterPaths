//;;
// Created by bernardoct on 8/26/17.
//

#include <fstream>
#include <iomanip>
#include <sys/stat.h>
#include "MasterDataCollector.h"
#include "../Utils/ObjectivesCalculator.h"
#include "../DroughtMitigationInstruments/Transfers.h"
#include "TransfersDataCollector.h"
#include "../SystemComponents/WaterSources/Quarry.h"
#include "../SystemComponents/WaterSources/WaterReuse.h"
#include "../SystemComponents/WaterSources/AllocatedReservoir.h"
#include "ReservoirDataCollector.h"
#include "IntakeDataCollector.h"
#include "QuaryDataCollector.h"
#include "WaterReuseDataCollector.h"
#include "AllocatedReservoirDataCollector.h"
#include "EmptyDataCollector.h"
#include "../DroughtMitigationInstruments/RawWaterReleases.h"
#include "RawWaterReleaseDataCollector.h"

using namespace Constants;

void MasterDataCollector::printPoliciesOutputCompact(
        int week_i, int week_f, string file_name) {

    if (!drought_mitigation_policy_collectors.empty()) {
#pragma omp parallel for
        for (int r = 0; r < drought_mitigation_policy_collectors[0].size();
             ++r) {
            std::ofstream out_stream;
            out_stream.open(output_directory + "/" + file_name + "_r"
                            + std::to_string(r) + ".csv");

            string line;
            for (vector<DataCollector *> p : drought_mitigation_policy_collectors)
                line += p[r]->printCompactStringHeader();
            line.pop_back();
            out_stream << line << endl;

            for (int w = week_i; w < week_f; ++w) {
                line = "";
                for (vector<DataCollector *> p : drought_mitigation_policy_collectors)
                    line += p[r]->printCompactString(w);
                line.pop_back();
                out_stream << line << endl;
            }

            out_stream.close();
        }
    }
}


void MasterDataCollector::printPoliciesOutputTabular(
        int week_i, int week_f, string file_name) {

    if (!drought_mitigation_policy_collectors.empty()) {
#pragma omp parallel for
        for (int r = 0; r < drought_mitigation_policy_collectors[0].size();
             ++r) {
            std::ofstream out_stream;
            out_stream.open(output_directory + "/" + file_name + "_r"
                            + std::to_string(r) + ".tab");

            out_stream << "    ";
            for (vector<DataCollector *> p : drought_mitigation_policy_collectors)
                out_stream << p[r]->printTabularStringHeaderLine1();
            out_stream << endl;

            out_stream << "Week";
            for (vector<DataCollector *> p : drought_mitigation_policy_collectors)
                out_stream << p[r]->printTabularStringHeaderLine2();
            out_stream << endl;

            for (int w = week_i; w < week_f; ++w) {
                out_stream << setw(4) << w;
                for (vector<DataCollector *> p : drought_mitigation_policy_collectors)
                    out_stream << p[r]->printTabularString(w);
                out_stream << endl;
            }

            out_stream.close();
        }
    }
}

void MasterDataCollector::printUtilitiesOutputCompact(
        int week_i, int week_f, string file_name) {

#pragma omp parallel for
    for (int r = 0; r < utility_collectors[0].size(); ++r) {
        std::ofstream out_stream;
        out_stream.open(output_directory + "/" + file_name + "_r"
                        + std::to_string(r) + ".csv");

        string line;
        for (vector<UtilitiesDataCollector> &p : utility_collectors)
            line += p[r].printCompactStringHeader();
        line.pop_back();
        out_stream << line << endl;

        for (int w = week_i; w < week_f; ++w) {
            line = "";
            for (vector<UtilitiesDataCollector> &p : utility_collectors)
                line += p[r].printCompactString(w);
            line.pop_back();
            out_stream << line << endl;
        }

        out_stream.close();
    }
}


void MasterDataCollector::printUtilitesOutputTabular(
        int week_i, int week_f, string file_name) {

#pragma omp parallel for
    for (int r = 0; r < utility_collectors[0].size(); ++r) {
        std::ofstream out_stream;
        out_stream.open(output_directory + "/" + file_name + "_r"
                        + std::to_string(r) + ".tab");

        stringstream names;
        names << "    ";
        for (vector<UtilitiesDataCollector> &p : utility_collectors)
            names << setw(p[0].table_width) << p[r].name;

        out_stream << names.str();
        out_stream << endl;

        out_stream << "    ";
        for (vector<UtilitiesDataCollector> &p : utility_collectors)
            out_stream << p[r].printTabularStringHeaderLine1();
        out_stream << endl;

        out_stream << "Week";
        for (vector<UtilitiesDataCollector> &p : utility_collectors)
            out_stream << p[r].printTabularStringHeaderLine2();
        out_stream << endl;

        for (int w = week_i; w < week_f; ++w) {
            out_stream << setw(4) << w;
            for (vector<UtilitiesDataCollector> &p : utility_collectors)
                out_stream << p[r].printTabularString(w);
            out_stream << endl;
        }

        out_stream.close();
    }
}

void MasterDataCollector::printWaterSourcesOutputCompact(
        int week_i, int week_f, string file_name) {

#pragma omp parallel for
    for (int r = 0; r < water_source_collectors[0].size(); ++r) {
        std::ofstream out_stream;
        out_stream.open(output_directory + "/" + file_name + "_r"
                        + std::to_string(r) + ".csv");

        string line;
        for (vector<DataCollector *> p : water_source_collectors)
            line += p[r]->printCompactStringHeader();
        line.pop_back();
        out_stream << line << endl;

        for (int w = week_i; w < week_f; ++w) {
            line = "";
            for (vector<DataCollector *> p : water_source_collectors)
                line += p[r]->printCompactString(w);
            line.pop_back();
            out_stream << line << endl;
        }

        out_stream.close();
    }
}


void MasterDataCollector::printWaterSourcesOutputTabular(
        int week_i, int week_f, string file_name) {

#pragma omp parallel for
    for (int r = 0; r < water_source_collectors[0].size(); ++r) {
        std::ofstream out_stream;
        out_stream.open(output_directory + "/" + file_name + "_r"
                        + std::to_string(r) + ".tab");

        stringstream names;
        names << "    ";
        for (vector<DataCollector *> p : water_source_collectors)
            names << setw(p[0]->table_width) << p[r]->name;

        out_stream << names.str();
        out_stream << endl;

        out_stream << "    ";
        for (vector<DataCollector *> p : water_source_collectors)
            out_stream << p[r]->printTabularStringHeaderLine1();
        out_stream << endl;

        out_stream << "Week";
        for (vector<DataCollector *> p : water_source_collectors)
            out_stream << p[r]->printTabularStringHeaderLine2();
        out_stream << endl;

        for (int w = week_i; w < week_f; ++w) {
            out_stream << setw(4) << w;
            for (vector<DataCollector *> p : water_source_collectors)
                out_stream << p[r]->printTabularString(w);
            out_stream << endl;
        }

        out_stream.close();
    }
}

vector<double> MasterDataCollector::calculatePrintObjectives(string file_name,
                                                             bool print) {

    vector<double> objectives;

    if (print) {
        cout << "Calculating and printing Objectives" << endl;
        //string obj_file_path = output_directory + "/" + file_name + ".out"; doesnt work on windows
        // cant have "/" in front of C:\ in the path name
        string obj_file_path = output_directory + "" + file_name + ".out";
        cout << obj_file_path << endl;

        std::ofstream outStream;

        outStream.open(obj_file_path);

        outStream << setw(COLUMN_WIDTH) << "      " << setw((COLUMN_WIDTH * 2))
                  << "Reliability"
                  << setw(COLUMN_WIDTH * 2) << "Restriction Freq."
                  //              << setw(COLUMN_WIDTH * 2) << "Jordan Lake Alloc."
                  << setw(COLUMN_WIDTH * 2) << "Infrastructure NPC"
                  << setw(COLUMN_WIDTH * 2) << "Peak Financial Cost"
                  << setw(COLUMN_WIDTH * 2) << "Worse Case Costs" << endl;

        cout << setw(COLUMN_WIDTH) << "      " << setw((COLUMN_WIDTH * 2))
                  << "Reliability"
                  << setw(COLUMN_WIDTH * 2) << "Restriction Freq."
                  //              << setw(COLUMN_WIDTH * 2) << "Jordan Lake Alloc."
                  << setw(COLUMN_WIDTH * 2) << "Infrastructure NPC"
                  << setw(COLUMN_WIDTH * 2) << "Peak Financial Cost"
                  << setw(COLUMN_WIDTH * 2) << "Worse Case Costs" << endl;

        for (auto &u : utility_collectors) {
            /// Create vector with restriction policies pertaining only to the
            /// utility whose objectives are being calculated.
            vector<RestrictionsDataCollector> restrictions;
            for (vector<DataCollector *> p : drought_mitigation_policy_collectors)
                if (p[0]->type == RESTRICTIONS && p[0]->id == u[0].id)
                    for (int i = 0; i < p.size(); ++i) {
                        restrictions.push_back(
                                dynamic_cast<RestrictionsDataCollector &>(*p[i]));
                    }

            /// Reliability
            double reliability = ObjectivesCalculator::calculateReliabilityObjective(u);
            /// Restriction Frequency
            double restriction_freq = ObjectivesCalculator::
            calculateRestrictionFrequencyObjective(restrictions);
            /// Infrastructure NPC
            double inf_npc = ObjectivesCalculator::
            calculateNetPresentCostInfrastructureObjective(u);
            /// Peak Financial Cost
            double financial_cost = ObjectivesCalculator::
            calculatePeakFinancialCostsObjective(u);
            /// Worse Case Costs
            double worse_cost = ObjectivesCalculator::calculateWorseCaseCostsObjective(u);

            outStream << setw(COLUMN_WIDTH) << u[0].name
                      /// Reliability
                      << setw(COLUMN_WIDTH * 2)
                      << setprecision(COLUMN_PRECISION)
                      << reliability
                      /// Restriction Frequency
                      << setw(COLUMN_WIDTH * 2)
                      << setprecision(COLUMN_PRECISION)
                      << restriction_freq
                      /// Infrastructure NPC
                      << setw(COLUMN_WIDTH * 2)
                      << setprecision(COLUMN_PRECISION)
                      << inf_npc
                      /// Peak Financial Cost
                      << setw(COLUMN_WIDTH * 2)
                      << setprecision(COLUMN_PRECISION)
                      << financial_cost
                      /// Worse Case Costs
                      << setw(COLUMN_WIDTH * 2)
                      << setprecision(COLUMN_PRECISION)
                      << worse_cost
                      << endl;

            cout << setw(COLUMN_WIDTH) << u[0].name
                      /// Reliability
                      << setw(COLUMN_WIDTH * 2)
                      << setprecision(COLUMN_PRECISION)
                      << reliability
                      /// Restriction Frequency
                      << setw(COLUMN_WIDTH * 2)
                      << setprecision(COLUMN_PRECISION)
                      << restriction_freq
                      /// Infrastructure NPC
                      << setw(COLUMN_WIDTH * 2)
                      << setprecision(COLUMN_PRECISION)
                      << inf_npc
                      /// Peak Financial Cost
                      << setw(COLUMN_WIDTH * 2)
                      << setprecision(COLUMN_PRECISION)
                      << financial_cost
                      /// Worse Case Costs
                      << setw(COLUMN_WIDTH * 2)
                      << setprecision(COLUMN_PRECISION)
                      << worse_cost
                      << endl;

            objectives.push_back(reliability);
            objectives.push_back(restriction_freq);
            objectives.push_back(inf_npc);
            objectives.push_back(financial_cost);
            objectives.push_back(worse_cost);
        }

        outStream.close();

        for (int i = 0; i < objectives.size(); ++i) {
            double o = objectives.at(i);
            if (o > 10e10 || o < -0.1) {
                char* error;
                sprintf(error, "Objective %d has absurd value of %f. Aborting.\n", i, o);
                throw_with_nested(runtime_error(error));
            }
        }
    } else {
        cout << "Calculating Objectives" << endl;
        for (auto &u : utility_collectors) {
            /// Create vector with restriction policies pertaining only to the
            /// utility whose objectives are being calculated.
            vector<RestrictionsDataCollector> utility_restrictions;
            for (auto &p : drought_mitigation_policy_collectors)
                if (p[0]->type == RESTRICTIONS && p[0]->id == u[0].id)
                    for (int i = 0; i < p.size(); ++i) {
                        utility_restrictions.push_back(
                                dynamic_cast<RestrictionsDataCollector &>(*p[i]));
                    }

            objectives.push_back
                    (ObjectivesCalculator::calculateReliabilityObjective(u));
            objectives.push_back
                    (ObjectivesCalculator::calculateRestrictionFrequencyObjective(utility_restrictions));
            objectives.push_back
                    (ObjectivesCalculator::calculateNetPresentCostInfrastructureObjective(u));
            objectives.push_back
                    (ObjectivesCalculator::calculatePeakFinancialCostsObjective(u));
            objectives.push_back
                    (ObjectivesCalculator::calculateWorseCaseCostsObjective(u));
        }
    }
    return objectives;
}

void MasterDataCollector::printPathways(string file_name) {
    std::ofstream outStream;
    outStream.open(output_directory + "/" + file_name + ".out");

    outStream << "Realization\tutility\tweek\tinfra." << endl;

    for (auto &uc : utility_collectors)
        for (int r = 0; r < uc.size(); ++r) {
            for (vector<int> infra : uc[r].getPathways()) {
                outStream << r << "\t" << infra[0] << "\t" << infra[1] << "\t"
                          << infra[2] << endl;
            }
        }

    outStream.close();

}

void MasterDataCollector::setOutputDirectory(string directory) {
    output_directory = directory;

    struct stat sb;
    if (stat(output_directory.c_str(), &sb) == 0)
        cout << "Output will be printed to folder " << output_directory << endl;
    else {
        cout << output_directory << endl;
        __throw_invalid_argument("Output folder does not exist.");
    }
}

void MasterDataCollector::addRealization(
        vector<WaterSource *> water_sources_realization,
        vector<DroughtMitigationPolicy *> drought_mitigation_policies_realization,
        vector<Utility *> utilities_realization,
        unsigned long r) {

    /// If collectors vectors have not yet been initialized, initialize them.
    if (water_source_collectors.empty()) {
        water_source_collectors = vector<vector<DataCollector *>>
                (water_sources_realization.size());
        drought_mitigation_policy_collectors = vector<vector<DataCollector *>>
                (drought_mitigation_policies_realization.size());
        utility_collectors = vector<vector<UtilitiesDataCollector>>
                (utilities_realization.size());
    }

 //   cout << "adding Utilities collector r " << r << endl;
    /// Create utilities data collectors
    for (int u = 0; u < utilities_realization.size(); ++u) {
        utility_collectors[u].push_back(UtilitiesDataCollector(utilities_realization[u], r));
    }
 //   cout << "done " << r << endl;

    /// Create drought mitigation policies data collector
    for (int dmp = 0; dmp < drought_mitigation_policies_realization.size();
         ++dmp)
        if (drought_mitigation_policies_realization[dmp]->type == RESTRICTIONS)
            drought_mitigation_policy_collectors[dmp].push_back(
                    new RestrictionsDataCollector(
                            dynamic_cast<Restrictions *> (drought_mitigation_policies_realization[dmp]), r));
        else if (drought_mitigation_policies_realization[dmp]->type == TRANSFERS)
            drought_mitigation_policy_collectors[dmp].push_back(
                    new TransfersDataCollector
                            (dynamic_cast<Transfers *> (drought_mitigation_policies_realization[dmp]), r));
        else if (drought_mitigation_policies_realization[dmp]->type == INSURANCE_STORAGE_ROF)
            drought_mitigation_policy_collectors[dmp].push_back(
                    new EmptyDataCollector());
        else if (drought_mitigation_policies_realization[dmp]->type ==
                 RAW_WATER_TRANSFERS)
            drought_mitigation_policy_collectors[dmp].push_back(
                    new RawWaterReleaseDataCollector
                            (dynamic_cast<RawWaterReleases *>
                             (drought_mitigation_policies_realization[dmp])));
        else
            __throw_invalid_argument("Drought mitigation policy not recognized. "
                                             "Did you forget to add it to the "
                                             "MasterDataCollector::addRealization"
                                             " function?");

    /// Create water sources data collectors
    for (int ws = 0; ws < water_sources_realization.size(); ++ws) {
        if (water_sources_realization[ws]->source_type == RESERVOIR)
            water_source_collectors[ws].push_back(
                    new ReservoirDataCollector(dynamic_cast<Reservoir *> (water_sources_realization[ws]), r));
        else if (water_sources_realization[ws]->source_type == INTAKE)
            water_source_collectors[ws].push_back(
                    new IntakeDataCollector(dynamic_cast<Intake *> (water_sources_realization[ws]), r));
        else if (water_sources_realization[ws]->source_type == QUARRY)
            water_source_collectors[ws].push_back(
                    new QuaryDataCollector(dynamic_cast<Quarry *> (water_sources_realization[ws]), r));
        else if (water_sources_realization[ws]->source_type == WATER_REUSE)
            water_source_collectors[ws].push_back(
                    new WaterReuseDataCollector(dynamic_cast<WaterReuse *> (water_sources_realization[ws]), r));
        else if (water_sources_realization[ws]->source_type == ALLOCATED_RESERVOIR)
            water_source_collectors[ws].push_back(
                    new AllocatedReservoirDataCollector(dynamic_cast<AllocatedReservoir *>
                                                                            (water_sources_realization[ws]), r));
        else if (water_sources_realization[ws]->source_type ==
                 RESERVOIR_EXPANSION ||
                 water_sources_realization[ws]->source_type ==
                 NEW_WATER_TREATMENT_PLANT ||
                 water_sources_realization[ws]->source_type ==
                 SOURCE_RELOCATION ||
                 water_sources_realization[ws]->source_type ==
                 ALLOCATED_RESERVOIR_EXPANSION)
            water_source_collectors[ws].push_back(
                    new EmptyDataCollector());
        else
            __throw_invalid_argument("Water source not recognized. "
                                             "Did you forget to add it to the "
                                             "MasterDataCollector::addRealization"
                                             " function?");
    }
}

void MasterDataCollector::collectData(unsigned long r) {

    for (vector<UtilitiesDataCollector> &uc : utility_collectors)
        uc[r].collect_data();

    for (vector<DataCollector *> dmp : drought_mitigation_policy_collectors)
        dmp[r]->collect_data();

    for (vector<DataCollector *> ws : water_source_collectors)
        ws[r]->collect_data();

}

MasterDataCollector::~MasterDataCollector() {

    for (vector<DataCollector *> dcs : water_source_collectors)
        for (DataCollector *dc : dcs)
            delete dc;

    for (vector<DataCollector *> dcs : drought_mitigation_policy_collectors)
        for (DataCollector *dc : dcs)
            delete dc;
}

MasterDataCollector::MasterDataCollector() {}
