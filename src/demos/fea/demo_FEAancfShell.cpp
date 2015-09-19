//
// PROJECT CHRONO - http://projectchrono.org
//
// Copyright (c) 2013 Project Chrono
// All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file at the top level of the distribution
// and at http://projectchrono.org/license-chrono.txt.
//

//   Demos code about
//
//     - FEA using ANCF (introduction to dynamics)
//
// Include some headers used by this tutorial...
#include "chrono/physics/ChSystem.h"
#include "chrono/physics/ChBodyEasy.h"
#include "chrono/lcp/ChLcpIterativeMINRES.h"
#include "chrono_fea/ChElementShellANCF.h"
#include "chrono_fea/ChMesh.h"
#include "chrono_fea/ChLinkPointFrame.h"
#include "chrono_fea/ChLinkDirFrame.h"
#include "chrono_fea/ChVisualizationFEAmesh.h"
#include "chrono_irrlicht/ChIrrApp.h"

// Remember to use the namespace 'chrono' because all classes
// of Chrono::Engine belong to this namespace and its children...

using namespace chrono;
using namespace fea;
using namespace irr;

int main(int argc, char* argv[]) {

	ChSystem my_system;

// Create the Irrlicht visualization (open the Irrlicht device,
	// bind a simple user interface, etc. etc.)
	ChIrrApp application(&my_system, L"ANCF Shells",
			core::dimension2d<u32>(800, 600), false, true);

	// Easy shortcuts to add camera, lights, logo and sky in Irrlicht scene:
	application.AddTypicalLogo();
	application.AddTypicalSky();
	application.AddTypicalLights();
	application.AddTypicalCamera(core::vector3df(0.2, 0, -1), // camera location
	core::vector3df(0.2, 0, 1));             // "look at" location
	GetLog()
			<< "\n----------------------------------------------------------------------------------------------\n";
	GetLog()
			<< "----------------------------------------------------------------------------------------------\n";
	GetLog() << "      ANCF Shell Elements demo with implicit integration \n";
	GetLog()
			<< "----------------------------------------------------------------------------------------------\n";

	// The physical system: it contains all physical objects.

	// Create a mesh, that is a container for groups
	// of elements and their referenced nodes.
	ChSharedPtr<ChMesh> my_mesh(new ChMesh);
	int numFlexBody = 1;
	// Geometry of the plate
	double plate_lenght_x = 1;
	double plate_lenght_y = 0.01;
	double plate_lenght_z = 0.1;  // small thickness
	// Specification of the mesh
	int numDiv_x = 10;
	int numDiv_y = 1;
	int numDiv_z = 1;
	int N_x = numDiv_x + 1;
	int N_y = numDiv_y + 1;
	int N_z = numDiv_z + 1;
	// Number of elements in the z direction is considered as 1
	int TotalNumElements = numDiv_x * numDiv_y;
	GetLog()
			<< "----------------------------------------------------------------------------------------------\n";
	GetLog() << "\tGeometry: A plate fixed at the left end \n";
	GetLog() << "\tPlate_lenght_x = " << plate_lenght_x << " Plate_lenght_y ="
			<< plate_lenght_y << "\n";
	GetLog()
			<< "\tMesh properties: Structured uniform mesh in each direction \n";
	GetLog() << "\tNumber of devision in x = " << numDiv_x
			<< " Number of devision in y =" << numDiv_y << "\n";
	GetLog()
			<< "----------------------------------------------------------------------------------------------\n";
	//(1+1) is the number of nodes in the z direction
	int TotalNumNodes = (numDiv_x + 1) * (numDiv_y + 1); // Or *(numDiv_z+1) for multilayer
	// For uniform mesh
	double dx = plate_lenght_x / numDiv_x;
	double dy = plate_lenght_y / numDiv_y;
	double dz = plate_lenght_z / numDiv_z;
	int MaxMNUM = 0;
	int MTYPE = 0;
	int MaxLayNum = 0;

	// There is a limitation now as follows
	//  Max #element is 1000;
	//  Max #Layer   is 75
	//  Only Orthotropic material
	ChMatrixDynamic<double> COORDFlex(TotalNumNodes, 6);
	ChMatrixDynamic<double> VELCYFlex(TotalNumNodes, 6);
	ChMatrixDynamic<double> NumNodes(TotalNumElements, 4);
	ChMatrixDynamic<int> LayNum(TotalNumElements, 1);
	ChMatrixDynamic<int> NDR(TotalNumNodes, 6);
	ChMatrixDynamic<double> ElemLengthXY(TotalNumElements, 2);
	ChMatrixNM<double, 10, 12> MPROP;
	ChMatrixNM<double, 10, 7> MNUM;
	ChMatrixNM<int, 10, 1> NumLayer;

	//ChMatrixNM<double> LayPROP(10, 7, 2);
	double LayPROP[10][7][2];

	//!-----------------------------------------------------------------------------------------------------------!
	//!------------------------------------- Elememt data    --------------------------------------------!
	//!-----------------------------------------------------------------------------------------------------------!
	GetLog() << "\tDefining element data and mesh ... \n";
	GetLog()
			<< "\tCreating the mesh based on number of nodes in each direction ...\n";

	for (int i = 0; i < TotalNumElements; i++) {
		// All the elements blong to the same layer, e.g layer number 1.
		LayNum(i, 0) = 1;
		// Node number of the 4 nodes which creates element i.
		// The nodes are distributed this way. First in the x direction for constant y when max x is reached go to the
		// next level for y by doing the same   distribution but for y+1 and keep doing until y limit is riched. Node
		// number start from 1.
		NumNodes(i, 0) = (i / (numDiv_x)) * (N_x) + i % numDiv_x;
		NumNodes(i, 1) = (i / (numDiv_x)) * (N_x) + i % numDiv_x + 1;
		NumNodes(i, 2) = (i / (numDiv_x)) * (N_x) + i % numDiv_x + N_x;
		NumNodes(i, 3) = (i / (numDiv_x)) * (N_x) + i % numDiv_x + 1 + N_x;
		if (i % (numDiv_x) == 0)
			GetLog()
					<< "\t- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n";

		// Let's keep the element length a fixed number in both direction. (uniform distribution of nodes in both
		// direction)
		// If the user interested in non-uniform mesh it can be manipulated here.

		ElemLengthXY(i, 0) = dx;
		ElemLengthXY(i, 1) = dz;
		//  ElemLengthXY(i,2)=plate_lenght_z/numDiv_z;

		GetLog() << "\tNodes of element " << i << " are  \t " << NumNodes(i, 0)
				<< "\t" << NumNodes(i, 1) << "\t" << NumNodes(i, 2) << "\t"
				<< NumNodes(i, 3) << "\n\t\t\n";

		if (MaxLayNum < LayNum(i, 0)) {
			MaxLayNum = LayNum(i, 0);
		}
	}
	GetLog()
			<< "----------------------------------------------------------------------------------------------\n";
	//!-----------------------------------------------------------------------------------------------------------!
	//!--------------------------------- NDR,COORDFlex,VELCYFlex ----------------------------!
	//!-----------------------------------------------------------------------------------------------------------!
	GetLog()
			<< "\tDefining initial coordinates and boundary conditZions ... \n";

	for (int i = 0; i < TotalNumNodes; i++) {
		// If the node is the first node from the left side fix the x,y,z degree of freedom+ d/dx, d/dy,d/dz as well. 1
		// for constrained 0 for ...
		//-The NDR array is used to define the degree of freedoms that are constrained in the 6 variable explained
		//above.
		NDR(i, 0) = (i % (numDiv_x + 1) == 0) ? 1 : 0;
		NDR(i, 1) = (i % (numDiv_x + 1) == 0) ? 1 : 0;
		NDR(i, 2) = (i % (numDiv_x + 1) == 0) ? 1 : 0;
		NDR(i, 3) = (i % (numDiv_x + 1) == 0) ? 1 : 0;
		NDR(i, 4) = (i % (numDiv_x + 1) == 0) ? 1 : 0;
		NDR(i, 5) = (i % (numDiv_x + 1) == 0) ? 1 : 0;
		GetLog() << "\t NODES = " << i << "\t\tx \ty\tz\n";
		GetLog() << "\t Position constrains\t" << NDR(i, 0) << "\t" << NDR(i, 1)
				<< "\t" << NDR(i, 2) << "\n \t Derivative constrains \t"
				<< NDR(i, 3) << "\t" << NDR(i, 4) << "\t" << NDR(i, 5) << "\n";

		//-COORDFlex are the initial coordinates for each node,
		// the first three are the position and the last three are the slope coordinates for the tangent vector.
		// Note that i starts from 0 but nodes starts from 1
		COORDFlex(i, 0) = (i % (numDiv_x + 1)) * dx;
//        COORDFlex(i,1) = (i / (numDiv_x + 1)) % (numDiv_y + 1) * dy;
//        COORDFlex(i,2) = (i) / ((numDiv_x + 1) * (numDiv_y + 1)) * dz;
		COORDFlex(i, 1) = (i) / ((numDiv_x + 1) * (numDiv_y + 1)) * dy;
		COORDFlex(i, 2) = (i / (numDiv_x + 1)) % (numDiv_y + 1) * dz;
		COORDFlex(i, 3) = 0;
		COORDFlex(i, 4) = 1;
		COORDFlex(i, 5) = 0;
		GetLog() << " \t Positions \t\t" << COORDFlex(i, 0) << "\t"
				<< COORDFlex(i, 1) << "\t" << COORDFlex(i, 2)
				<< "\n \t Slope \t\t\t" << COORDFlex(i, 3) << "\t"
				<< COORDFlex(i, 4) << "\t" << COORDFlex(i, 5) << "\n";

		//-VELCYFlex iss essentially the same as COORDFlex, but for the initial velocity instead of position.
		// let's assume zero initilal velocity for nodes in the
		VELCYFlex(i, 0) = 0;
		VELCYFlex(i, 1) = 0;
		VELCYFlex(i, 2) = 0;
		VELCYFlex(i, 3) = 0;
		VELCYFlex(i, 4) = 0;
		VELCYFlex(i, 5) = 0;
		GetLog() << "\t Velocities\t\t" << VELCYFlex(i, 0) << "\t"
				<< VELCYFlex(i, 1) << "\t" << VELCYFlex(i, 2)
				<< "\n\t Slopes V\t\t" << VELCYFlex(i, 3) << "\t"
				<< VELCYFlex(i, 4) << "\t" << VELCYFlex(i, 5) << "\n\n";
	}

	//!-----------------------------------------------------------------------------------------------------------!
	//!------------------------------------ Read Layer Data --------------------------------------------!
	//!-----------------------------------------------------------------------------------------------------------!
	GetLog()
			<< "----------------------------------------------------------------------------------------------\n";
	GetLog() << "\tDefining layers and material properties ... \n";
	for (int i = 0; i < MaxLayNum; i++) {
		NumLayer(i, 0) = i + 1;
		// For each layer define some properties
		// For multilayer problem the user should modify the following loop as they would like
		for (int j = 0; j < NumLayer(i, 0); j++) {
			LayPROP[i][j][0] = dy;  // Layerheight
			LayPROP[i][j][1] = 0;  // PlyAngle
			MNUM[i][j] = 1;  // Material_ID
			if (MaxMNUM < MNUM(i, j))
				MaxMNUM = MNUM(i, j);
		}
	}

	//!-----------------------------------------------------------------------------------------------------------!
	//!--------------------------------- Read Material Data ------------------------------------------!
	//!-----------------------------------------------------------------------------------------------------------!

	for (int i = 0; i < MaxMNUM; i++) {
		MTYPE = 2;

		if (MTYPE == 1) {
			MPROP(i, 0) = 500;  // Density [kg/m3]
			MPROP(i, 1) = 2.1E+07;  // H(m)
			MPROP(i, 2) = 2.1E+07;  // E(Pa)/nu
			MPROP(i, 3) = 2.1E+07;  // G(Pa)
		}

		if (MTYPE == 2) {
			MPROP(i, 0) = 500;  // Density [kg/m3]
			MPROP(i, 1) = 2.1E+07;  // Ex
			MPROP(i, 2) = 2.1E+07;  // Ey
			MPROP(i, 3) = 2.1E+07;  // Ez
			// Additional information for the Type 2 of Material.
			MPROP(i, 4) = 0.3;  // nuxy
			MPROP(i, 5) = 0.3;  // nuxz
			MPROP(i, 6) = 0.3;  // nuyz
			MPROP(i, 7) = 8.0769231E+06;  // Gxy
			MPROP(i, 8) = 8.0769231E+06;  // Gxz
			MPROP(i, 9) = 8.0769231E+06;  // Gyz
		}
	}

	GetLog()
			<< "----------------------------------------------------------------------------------------------\n";
	GetLog() << "\tDone with pre-processing ... \n";
	GetLog()
			<< "----------------------------------------------------------------------------------------------\n";
	ChSharedPtr<ChContinuumElastic> mmaterial(new ChContinuumElastic);
	mmaterial->Set_RayleighDampingK(0.0);
	mmaterial->Set_RayleighDampingM(0.0);

	int i = 0;
	while (i < TotalNumNodes) {
		ChSharedPtr<ChNodeFEAxyzD> node(
				new ChNodeFEAxyzD(
						ChVector<>(COORDFlex(i, 0), COORDFlex(i, 1),
								COORDFlex(i, 2)),
						ChVector<>(COORDFlex(i, 3), COORDFlex(i, 4),
								COORDFlex(i, 5))));
		node->SetMass(0.0);
		my_mesh->AddNode(node);
		if (NDR(i, 0) == 1 && NDR(i, 1) == 1 && NDR(i, 2) == 1 && NDR(i, 3) == 1
				&& NDR(i, 4) == 1 && NDR(i, 5) == 1) {
			node->SetFixed(true);
		}
		i++;
	}

	GetLog()
			<< "\tNodal data of a node with the following information is recorded\n";
	ChSharedPtr<ChNodeFEAxyzD> nodetip(
			my_mesh->GetNode(TotalNumNodes - 1).DynamicCastTo<ChNodeFEAxyzD>());
	GetLog() << "\tX = " << nodetip->GetPos().x << " Y = "
			<< nodetip->GetPos().y << " Z = " << nodetip->GetPos().z << "\n";
	GetLog() << "\tdX = " << nodetip->GetD().x << " dY = " << nodetip->GetD().y
			<< " dZ = " << nodetip->GetD().z << "\n";
	GetLog()
			<< "----------------------------------------------------------------------------------------------\n";
	int elemcount = 0;
	while (elemcount < TotalNumElements) {
		ChSharedPtr<ChElementShellANCF> element(new ChElementShellANCF);
		// Save material data into InertFlexVec(98x1) at each layer
		ChMatrixNM<double, 98, 1> InertFlexVec;
		InertFlexVec.Reset();
		double TotalThickness;  // element thickness
		TotalThickness = 0.0;
		int i = elemcount;
		for (int j = 0; j < NumLayer(LayNum(i, 0) - 1, 0); j++) {
			int ij = 14 * j;
			InertFlexVec(ij) = MPROP[MNUM[LayNum(i, 0) - 1][j] - 1][0]; // Density
			InertFlexVec(ij + 1) = ElemLengthXY(i, 0);                // EL
			InertFlexVec(ij + 2) = ElemLengthXY(i, 1);                // EW
			InertFlexVec(ij + 3) = LayPROP[LayNum(i, 0) - 1][j][0]; // Thickness per layer
			TotalThickness += InertFlexVec(ij + 3);
			InertFlexVec(ij + 4) = LayPROP[LayNum(i, 0) - 1][j][1]; // Fiber angle
			InertFlexVec(ij + 5) = MPROP[MNUM[LayNum(i, 0) - 1][j] - 1][1]; // Ex
			InertFlexVec(ij + 6) = MPROP[MNUM[LayNum(i, 0) - 1][j] - 1][2]; // Ey
			InertFlexVec(ij + 7) = MPROP[MNUM[LayNum(i, 0) - 1][j] - 1][3]; // Ez
			InertFlexVec(ij + 8) = MPROP[MNUM[LayNum(i, 0) - 1][j] - 1][4]; // nuxy
			InertFlexVec(ij + 9) = MPROP[MNUM[LayNum(i, 0) - 1][j] - 1][5]; // nuxz
			InertFlexVec(ij + 10) = MPROP[MNUM[LayNum(i, 0) - 1][j] - 1][6]; // nuyz
			InertFlexVec(ij + 11) = MPROP[MNUM[LayNum(i, 0) - 1][j] - 1][7]; // Gxy
			InertFlexVec(ij + 12) = MPROP[MNUM[LayNum(i, 0) - 1][j] - 1][8]; // Gxz
			InertFlexVec(ij + 13) = MPROP[MNUM[LayNum(i, 0) - 1][j] - 1][9]; // Gyz
		}

		ChMatrixNM<double, 7, 2> GaussZRange;
		GaussZRange.Reset();
		double CurrentHeight = 0.0;
		for (int j = 0; j < NumLayer(LayNum(i, 0) - 1, 0); j++) {
			double AA = (CurrentHeight / TotalThickness - 0.5) * 2.0;
			CurrentHeight += LayPROP[LayNum(i, 0) - 1][j][0];
			double AAA = (CurrentHeight / TotalThickness - 0.5) * 2.0;
			GaussZRange(j, 0) = AA;
			GaussZRange(j, 1) = AAA;
		}
		// GetLog() << "GaussZRange" << GaussZRange;
		//
		element->SetInertFlexVec(InertFlexVec);
		element->SetGaussZRange(GaussZRange);
		element->SetNodes(
				my_mesh->GetNode(NumNodes[elemcount][0]).DynamicCastTo<
						ChNodeFEAxyzD>(),
				my_mesh->GetNode(NumNodes[elemcount][1]).DynamicCastTo<
						ChNodeFEAxyzD>(),
				my_mesh->GetNode(NumNodes[elemcount][2]).DynamicCastTo<
						ChNodeFEAxyzD>(),
				my_mesh->GetNode(NumNodes[elemcount][3]).DynamicCastTo<
						ChNodeFEAxyzD>());
		element->SetMaterial(mmaterial);
		element->SetNumLayer(NumLayer(LayNum(i, 0) - 1, 0));
		element->SetThickness(TotalThickness);
		element->SetElemNum(elemcount);
		element->SetAlphaDamp(0.00);
		//== 7/14/2015
		element->Setdt(0.001);             // dt to calculate DampingCoefficient
		element->SetGravityY(1);            // 0:No Gravity, 1:Gravity(Fz=-9.81)
		element->SetAirPressure(0);   // 0:No AirPressure, 1:220kPa Air Pressure
		ChMatrixNM<double, 35, 1> StockAlpha_EAS; // StockAlpha(5*7,1): Max #Layer is 7
		StockAlpha_EAS.Reset();
		element->SetStockAlpha(StockAlpha_EAS);
		my_mesh->AddElement(element);
		elemcount++;
	}

	// This is mandatory
	my_mesh->SetupInitial();

	// Remember to add the mesh to the system!
	my_system.Add(my_mesh);

//    ChSharedPtr<ChVisualizationFEAmesh> mvisualizebeamC(new ChVisualizationFEAmesh(*(my_mesh.get_ptr())));
//    mvisualizebeamC->SetFEMglyphType(ChVisualizationFEAmesh::E_GLYPH_NODE_CSYS);
//    mvisualizebeamC->SetFEMdataType(ChVisualizationFEAmesh::E_PLOT_NONE);
//    mvisualizebeamC->SetSymbolsThickness(0.006);
//    mvisualizebeamC->SetSymbolsScale(0.01);
//    mvisualizebeamC->SetZbufferHide(false);
//    my_mesh->AddAsset(mvisualizebeamC);

	ChSharedPtr<ChVisualizationFEAmesh> mvisualizemesh(
	new ChVisualizationFEAmesh(*(my_mesh.get_ptr())));
	mvisualizemesh->SetFEMdataType(
			ChVisualizationFEAmesh::E_PLOT_NODE_SPEED_NORM);
	mvisualizemesh->SetColorscaleMinMax(0.0, 5.50);
	mvisualizemesh->SetShrinkElements(true, 0.85);
	mvisualizemesh->SetSmoothFaces(true);
	my_mesh->AddAsset(mvisualizemesh);



	ChSharedPtr<ChVisualizationFEAmesh> mvisualizemeshref(
			new ChVisualizationFEAmesh(*(my_mesh.get_ptr())));
	mvisualizemeshref->SetFEMdataType(ChVisualizationFEAmesh::E_PLOT_SURFACE);
	mvisualizemeshref->SetWireframe(true);
	mvisualizemeshref->SetDrawInUndeformedReference(true);
	my_mesh->AddAsset(mvisualizemeshref);

	ChSharedPtr<ChVisualizationFEAmesh> mvisualizemeshC(
			new ChVisualizationFEAmesh(*(my_mesh.get_ptr())));
	mvisualizemeshC->SetFEMglyphType(
	ChVisualizationFEAmesh::E_GLYPH_NODE_DOT_POS);
	mvisualizemeshC->SetFEMdataType(ChVisualizationFEAmesh::E_PLOT_NONE);
	mvisualizemeshC->SetSymbolsThickness(0.004);
	my_mesh->AddAsset(mvisualizemeshC);


	 ChSharedPtr<ChVisualizationFEAmesh> mvisualizemeshD(new ChVisualizationFEAmesh(*(my_mesh.get_ptr())));
	 //mvisualizemeshD->SetFEMglyphType(ChVisualizationFEAmesh::E_GLYPH_NODE_VECT_SPEED);
	 mvisualizemeshD->SetFEMglyphType(ChVisualizationFEAmesh::E_GLYPH_ELEM_TENS_STRAIN);
	 mvisualizemeshD->SetFEMdataType(ChVisualizationFEAmesh::E_PLOT_NONE);
	 mvisualizemeshD->SetSymbolsScale(1);
	 mvisualizemeshD->SetColorscaleMinMax(-0.5,5);
	 mvisualizemeshD->SetZbufferHide(false);
	 my_mesh->AddAsset(mvisualizemeshD);


	// ==IMPORTANT!== Use this function for adding a ChIrrNodeAsset to all items
	// in the system. These ChIrrNodeAsset assets are 'proxies' to the Irrlicht meshes.
	// If you need a finer control on which item really needs a visualization proxy in
	// Irrlicht, just use application.AssetBind(myitem); on a per-item basis.
	application.AssetBindAll();

	// ==IMPORTANT!== Use this function for 'converting' into Irrlicht meshes the assets
	// that you added to the bodies into 3D shapes, they can be visualized by Irrlicht!

	application.AssetUpdateAll();

	i = 0;
	// Perform a dynamic time integration:
	my_system.SetLcpSolverType(ChSystem::LCP_ITERATIVE_MINRES); // <- NEEDED because other solvers can't handle stiffness matrices
	chrono::ChLcpIterativeMINRES* msolver =
			(chrono::ChLcpIterativeMINRES*) my_system.GetLcpSolverSpeed();
	msolver->SetDiagonalPreconditioning(true);
	my_system.SetIterLCPmaxItersSpeed(100);
	my_system.SetTolForce(1e-10);

	my_system.SetIntegrationType(ChSystem::INT_HHT); // INT_HHT);//INT_EULER_IMPLICIT);
	ChSharedPtr<ChTimestepperHHT> mystepper =
			my_system.GetTimestepper().DynamicCastTo<ChTimestepperHHT>();
	mystepper->SetAlpha(-0.2);
	mystepper->SetMaxiters(100);
	mystepper->SetTolerance(1e-5);
	mystepper->SetTolerance(1e-5);
	mystepper->SetMode(ChTimestepperHHT::POSITION);
	mystepper->SetScaling(true);
	double start = std::clock();

///*
	application.SetTimestep(0.001);
	while (application.GetDevice()->run()) {
		application.BeginScene();

		application.DrawAll();

		application.DoStep();
		GetLog() << " t=  " << my_system.GetChTime() << "\tx= "
				<< nodetip->GetPos().x << "\ty= " << nodetip->GetPos().y
				<< "\tz= " << nodetip->GetPos().z << "\n\n";
		double duration = (std::clock() - start) / (double) CLOCKS_PER_SEC;
		GetLog() << "Simulation Time: " << duration << "\n";

		application.EndScene();

	}
//*/
	int Iterations = 0;
	double timestep = 0.001;
	/*
	 while (my_system.GetChTime() < 0.03) {
	 my_system.DoStepDynamics(timestep);
	 Iterations += mystepper->GetNumIterations();

	 GetLog() << " t=  " << my_system.GetChTime() << "\tx= " << nodetip->GetPos().x << "\ty= " << nodetip->GetPos().y
	 << "\tz= " << nodetip->GetPos().z << "\n\n";
	 }
	 */

	double duration = (std::clock() - start) / (double) CLOCKS_PER_SEC;

	GetLog() << "Simulation Time: " << duration << "\n";

	return 0;
}
