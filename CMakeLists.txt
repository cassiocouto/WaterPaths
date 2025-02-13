cmake_minimum_required(VERSION 3.6)
project(TriangleModel)

set(CMAKE_CXX_STANDARD 14)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -march=native")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp -Og -march=native")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp -O0 -march=native")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp -O2 -march=native")
#set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static -ldl -lpthread")
#set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static -lpthread")

include_directories(/usr/include/mpi/)

set(SOURCE_FILES
		src/Utils/StdAfx.cpp
        src/Utils/StdAfx.h
		src/Utils/XGetopt.cpp
        src/Utils/XGetopt.h
        src/ContinuityModels/Base/ContinuityModel.cpp
        src/ContinuityModels/Base/ContinuityModel.h
        src/ContinuityModels/ContinuityModelRealization.cpp
        src/ContinuityModels/ContinuityModelRealization.h
        src/ContinuityModels/ContinuityModelROF.cpp
        src/ContinuityModels/ContinuityModelROF.h
        src/Controls/Base/MinEnvFlowControl.cpp
        src/Controls/Base/MinEnvFlowControl.h
        src/Controls/Custom/JordanLakeMinEnvFlowControl.cpp
        src/Controls/Custom/JordanLakeMinEnvFlowControl.h
        src/Controls/Custom/FallsLakeMinEnvFlowControl.cpp
        src/Controls/Custom/FallsLakeMinEnvFlowControl.h
        src/Controls/InflowMinEnvFlowControl.cpp
        src/Controls/InflowMinEnvFlowControl.h
        src/Controls/SeasonalMinEnvFlowControl.cpp
        src/Controls/SeasonalMinEnvFlowControl.h
        src/Controls/StorageMinEnvFlowControl.cpp
        src/Controls/StorageMinEnvFlowControl.h
        src/Controls/FixedMinEnvFlowControl.cpp
        src/Controls/FixedMinEnvFlowControl.h
        src/Controls/WwtpDischargeRule.cpp
        src/Controls/WwtpDischargeRule.h
        src/DroughtMitigationInstruments/Base/DroughtMitigationPolicy.cpp
        src/DroughtMitigationInstruments/Base/DroughtMitigationPolicy.h
        #        src/DroughtMitigationInstruments/InsurancePseudoROF.cpp
        #        src/DroughtMitigationInstruments/InsurancePseudoROF.h
        src/DroughtMitigationInstruments/InsuranceStorageToROF.cpp
        src/DroughtMitigationInstruments/InsuranceStorageToROF.h
        src/DroughtMitigationInstruments/Restrictions.cpp
        src/DroughtMitigationInstruments/Restrictions.h
        src/DroughtMitigationInstruments/Transfers.cpp
        src/DroughtMitigationInstruments/Transfers.h
        src/main.cpp
        src/Simulation/Simulation.cpp
        src/Simulation/Simulation.h
        src/SystemComponents/Utility/InfrastructureManager.cpp
        src/SystemComponents/Utility/InfrastructureManager.h
        src/SystemComponents/Catchment.cpp
        src/SystemComponents/Catchment.h
        src/SystemComponents/Utility/Utility.cpp
        src/SystemComponents/Utility/Utility.h
        src/SystemComponents/WaterSources/Base/WaterSource.cpp
        src/SystemComponents/WaterSources/Base/WaterSource.h
        src/SystemComponents/WaterSources/AllocatedReservoir.cpp
        src/SystemComponents/WaterSources/AllocatedReservoir.h
        src/SystemComponents/WaterSources/Intake.cpp
        src/SystemComponents/WaterSources/Intake.h
        src/SystemComponents/WaterSources/Quarry.cpp
        src/SystemComponents/WaterSources/Quarry.h
        src/SystemComponents/WaterSources/Relocation.cpp
        src/SystemComponents/WaterSources/Relocation.h
        src/SystemComponents/WaterSources/SequentialJointTreatmentExpansion.cpp
        src/SystemComponents/WaterSources/SequentialJointTreatmentExpansion.h
        src/SystemComponents/WaterSources/Reservoir.cpp
        src/SystemComponents/WaterSources/Reservoir.h
        src/SystemComponents/WaterSources/ReservoirExpansion.cpp
        src/SystemComponents/WaterSources/ReservoirExpansion.h
        src/SystemComponents/WaterSources/WaterReuse.cpp
        src/SystemComponents/WaterSources/WaterReuse.h
        src/DataCollector/Base/DataCollector.cpp
        src/DataCollector/Base/DataCollector.h
        src/DataCollector/AllocatedReservoirDataCollector.cpp
        src/DataCollector/AllocatedReservoirDataCollector.h
        src/DataCollector/EmptyDataCollector.cpp
        src/DataCollector/EmptyDataCollector.h
        src/DataCollector/IntakeDataCollector.cpp
        src/DataCollector/IntakeDataCollector.h
        src/DataCollector/MasterDataCollector.cpp
        src/DataCollector/MasterDataCollector.h
        src/DataCollector/QuaryDataCollector.cpp
        src/DataCollector/QuaryDataCollector.h
        src/DataCollector/ReservoirDataCollector.cpp
        src/DataCollector/ReservoirDataCollector.h
        src/DataCollector/RestrictionsDataCollector.cpp
        src/DataCollector/RestrictionsDataCollector.h
        src/DataCollector/TransfersDataCollector.cpp
        src/DataCollector/TransfersDataCollector.h
        src/DataCollector/UtilitiesDataCollector.h
        src/DataCollector/UtilitiesDataCollector.cpp
        src/DataCollector/WaterReuseDataCollector.cpp
        src/DataCollector/WaterReuseDataCollector.h
        src/Utils/Constants.h
        src/Utils/DataSeries.cpp
        src/Utils/DataSeries.h
        src/Controls/EvaporationSeries.cpp
        src/Controls/EvaporationSeries.h
        src/Utils/Graph/Graph.cpp
        src/Utils/Graph/Graph.h
        src/Utils/ObjectivesCalculator.cpp
        src/Utils/ObjectivesCalculator.h
        src/Utils/QPSolver/Array.cpp
        src/Utils/QPSolver/Array.h
        src/Utils/QPSolver/QuadProg++.cpp
        src/Utils/QPSolver/QuadProg++.h
        src/Utils/Utils.cpp
        src/Utils/Utils.h
        src/Problem/Triangle.cpp
        src/Problem/Triangle.h
        src/Problem/Base/Problem.cpp
        src/Problem/Base/Problem.h
        src/SystemComponents/Bonds/Base/Bond.cpp
        src/SystemComponents/Bonds/Base/Bond.h
        src/SystemComponents/Bonds/LevelDebtServiceBond.cpp
        src/SystemComponents/Bonds/LevelDebtServiceBond.h
        src/SystemComponents/Bonds/BalloonPaymentBond.cpp
        src/SystemComponents/Bonds/BalloonPaymentBond.h
        src/SystemComponents/Bonds/FloatingInterestBalloonPaymentBond.cpp
        src/SystemComponents/Bonds/FloatingInterestBalloonPaymentBond.h
        src/Problem/PaperTestProblem.cpp
        src/Problem/PaperTestProblem.h)

set(TEST_SOURCE_FILES
		src/Utils/StdAfx.cpp
        src/Utils/StdAfx.h
		src/Utils/XGetopt.cpp
        src/Utils/XGetopt.h
        src/ContinuityModels/Base/ContinuityModel.cpp
        src/ContinuityModels/Base/ContinuityModel.h
        src/ContinuityModels/ContinuityModelRealization.cpp
        src/ContinuityModels/ContinuityModelRealization.h
        src/ContinuityModels/ContinuityModelROF.cpp
        src/ContinuityModels/ContinuityModelROF.h
        src/Controls/Base/MinEnvFlowControl.cpp
        src/Controls/Base/MinEnvFlowControl.h
        src/Controls/Custom/JordanLakeMinEnvFlowControl.cpp
        src/Controls/Custom/JordanLakeMinEnvFlowControl.h
        src/Controls/Custom/FallsLakeMinEnvFlowControl.cpp
        src/Controls/Custom/FallsLakeMinEnvFlowControl.h
        src/Controls/InflowMinEnvFlowControl.cpp
        src/Controls/InflowMinEnvFlowControl.h
        src/Controls/SeasonalMinEnvFlowControl.cpp
        src/Controls/SeasonalMinEnvFlowControl.h
        src/Controls/StorageMinEnvFlowControl.cpp
        src/Controls/StorageMinEnvFlowControl.h
        src/Controls/FixedMinEnvFlowControl.cpp
        src/Controls/FixedMinEnvFlowControl.h
        src/Controls/WwtpDischargeRule.cpp
        src/Controls/WwtpDischargeRule.h
        src/DroughtMitigationInstruments/Base/DroughtMitigationPolicy.cpp
        src/DroughtMitigationInstruments/Base/DroughtMitigationPolicy.h
        src/DroughtMitigationInstruments/InsuranceStorageToROF.cpp
        src/DroughtMitigationInstruments/InsuranceStorageToROF.h
        src/DroughtMitigationInstruments/Restrictions.cpp
        src/DroughtMitigationInstruments/Restrictions.h
        src/DroughtMitigationInstruments/Transfers.cpp
        src/DroughtMitigationInstruments/Transfers.h
        src/Simulation/Simulation.cpp
        src/Simulation/Simulation.h
        src/SystemComponents/Utility/InfrastructureManager.cpp
        src/SystemComponents/Utility/InfrastructureManager.h
        src/SystemComponents/Catchment.cpp
        src/SystemComponents/Catchment.h
        src/SystemComponents/Utility/Utility.cpp
        src/SystemComponents/Utility/Utility.h
        src/SystemComponents/WaterSources/Base/WaterSource.cpp
        src/SystemComponents/WaterSources/Base/WaterSource.h
        src/SystemComponents/WaterSources/AllocatedReservoir.cpp
        src/SystemComponents/WaterSources/AllocatedReservoir.h
        src/SystemComponents/WaterSources/Intake.cpp
        src/SystemComponents/WaterSources/Intake.h
        src/SystemComponents/WaterSources/Quarry.cpp
        src/SystemComponents/WaterSources/Quarry.h
        src/SystemComponents/WaterSources/Relocation.cpp
        src/SystemComponents/WaterSources/Relocation.h
        src/SystemComponents/WaterSources/SequentialJointTreatmentExpansion.cpp
        src/SystemComponents/WaterSources/SequentialJointTreatmentExpansion.h
        src/SystemComponents/WaterSources/Reservoir.cpp
        src/SystemComponents/WaterSources/Reservoir.h
        src/SystemComponents/WaterSources/ReservoirExpansion.cpp
        src/SystemComponents/WaterSources/ReservoirExpansion.h
        src/SystemComponents/WaterSources/WaterReuse.cpp
        src/SystemComponents/WaterSources/WaterReuse.h
        src/DataCollector/Base/DataCollector.cpp
        src/DataCollector/Base/DataCollector.h
        src/DataCollector/AllocatedReservoirDataCollector.cpp
        src/DataCollector/AllocatedReservoirDataCollector.h
        src/DataCollector/EmptyDataCollector.cpp
        src/DataCollector/EmptyDataCollector.h
        src/DataCollector/IntakeDataCollector.cpp
        src/DataCollector/IntakeDataCollector.h
        src/DataCollector/MasterDataCollector.cpp
        src/DataCollector/MasterDataCollector.h
        src/DataCollector/QuaryDataCollector.cpp
        src/DataCollector/QuaryDataCollector.h
        src/DataCollector/ReservoirDataCollector.cpp
        src/DataCollector/ReservoirDataCollector.h
        src/DataCollector/RestrictionsDataCollector.cpp
        src/DataCollector/RestrictionsDataCollector.h
        src/DataCollector/TransfersDataCollector.cpp
        src/DataCollector/TransfersDataCollector.h
        src/DataCollector/UtilitiesDataCollector.h
        src/DataCollector/UtilitiesDataCollector.cpp
        src/DataCollector/WaterReuseDataCollector.cpp
        src/DataCollector/WaterReuseDataCollector.h
        src/Utils/Constants.h
        src/Utils/DataSeries.cpp
        src/Utils/DataSeries.h
        src/Controls/EvaporationSeries.cpp
        src/Controls/EvaporationSeries.h
        src/Utils/Graph/Graph.cpp
        src/Utils/Graph/Graph.h
        src/Utils/ObjectivesCalculator.cpp
        src/Utils/ObjectivesCalculator.h
        src/Utils/QPSolver/Array.cpp
        src/Utils/QPSolver/Array.h
        src/Utils/QPSolver/QuadProg++.cpp
        src/Utils/QPSolver/QuadProg++.h
        src/Utils/Utils.cpp
        src/Utils/Utils.h
        src/Problem/Triangle.cpp
        src/Problem/Triangle.h
        src/Problem/Base/Problem.cpp
        src/Problem/Base/Problem.h
        src/SystemComponents/Bonds/Base/Bond.cpp
        src/SystemComponents/Bonds/Base/Bond.h
        src/SystemComponents/Bonds/LevelDebtServiceBond.cpp
        src/SystemComponents/Bonds/LevelDebtServiceBond.h
        src/SystemComponents/Bonds/BalloonPaymentBond.cpp
        src/SystemComponents/Bonds/BalloonPaymentBond.h
        src/SystemComponents/Bonds/FloatingInterestBalloonPaymentBond.cpp
        src/SystemComponents/Bonds/FloatingInterestBalloonPaymentBond.h
        Tests/catch.hpp
        Tests/Tests.cpp)

add_executable(TriangleModel ${SOURCE_FILES})
add_executable(TestTriangleModel ${TEST_SOURCE_FILES})
