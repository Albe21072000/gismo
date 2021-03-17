/** @file Reissner-Mindlin_example.cpp

    @brief Tutorial on how to use expression assembler to solve the Poisson equation

    This file is part of the G+Smo library.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.

    Author(s): Yang Xia & Hugo Verhelst & HS. Wang
*/
#include <iostream>
#include <gismo.h>
#include "gsRMShell/gsRMShellAssembler.hpp"
#include "gsRMShell/gsPeridynamic.hpp"
#include "gsRMShell/gsHyperMeshOut.h"
#include "gsRMShell/gsUmfSolver.h"

using namespace std;
using namespace gismo;
using namespace DLUT::SAE::PERIDYNAMIC;

int LOAD_STEP = 1;
double RATIO_OF_HORIZON_MESHSIZE = 2.0;
double HORIZON = 3.0;
bool STRAINS = false; // ����Ӧ��Ӧ���

int main(int argc, char* argv[])
{
	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	// 1.��ʼ�� --------------------------------------------------------------------
	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	// Input options
	int		numElevate = 0;    //p ϸ��������
	int		numHrefine = 0;	//h ϸ�������
	int		plot = 1;    // �Ƿ����paraviewͼ��
	int		testCase = 6;    // ����ѡ��
	bool	nonlinear = false;    // ������
	int		boolPatchTest = false;    // ��Ƭ����
	int		memIntegPtAdd = 0;    // ��ӵĻ��ֵ�ĸ���(Ĭ��gauss���ֵ���=�������״�)
	int     result = 0;
	int		dof_node = 6;	// �ڵ����ɶ�

	Material m_material;
	gsStopwatch clock;	// ��ʱ��

	string input("../../TestCase/egg.xml");			// ģ�������ļ�
	string bc_file("../../TestCase/boundary.txt");  // �߽����������ļ�
	string output("../../TestResult/result.txt");   // �������ļ�

	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	// 2.���ݶ��� -------------------------------------------------------------------
	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

	// �ļ���ȡ����A.�����в���������Ŀ���Ҽ����ԡ����ԡ������в����������޸ģ�����Ҫ��Ϊcmd�˲����ṩ����
	// ���ڸ������е�ʹ����� https://gismo.github.io/commandLineArg_example.html
	gsCmdLine cmd("This program provides R-M shell numerical examples, give me your command!");
	cmd.addInt("t", "testcase", "Choose a test case. ", testCase);
	cmd.addInt("r", "hRefine", "Number of h-refinement steps", numHrefine);
	cmd.addInt("e", "degreeElevation", "Number of degree elevation steps", numElevate);
	cmd.addInt("i", "integration", "Number of added integration point", memIntegPtAdd);
	cmd.addInt("p", "plot", "Plot result in ParaView format", plot);
	cmd.addSwitch("n", "nonlinear", "Nonlinear elasticity (otherwise linear)", nonlinear); //boolean
	cmd.addString("g", "geometry", "InputFile containing Geometry (.xml, .axl, .txt)", input);
	cmd.addString("m", "name", "Output file name", output);
	cmd.addString("c", "boundary", "boundary condition", bc_file);
	// Read the arguments and update with the inputs, if given.
	try { cmd.getValues(argc, argv); }
	catch (int rv) { return rv; }
	// Print out the version information
	cmd.printVersion();
	gsInfo << "\nPrinting command line arguments:\n\n\n"
		<< "testCase:\t\t" << testCase << "\n"
		<< "numHref:\t\t" << numHrefine << "\n"
		<< "numElevate:\t\t" << numElevate << "\n"
		<< "memIntegPtAdd:\t\t" << memIntegPtAdd << "\n"
		<< "plot:\t\t\t" << plot << "\n"
		<< "nonlinear:\t\t" << numElevate << "\n"
		<< "input:\t\t\t" << input << "\n"
		<< "name_output:\t\t" << output << "\n"
		<< "bc_file:\t\t" << bc_file << "\n";

	// �ļ���ȡ����B.����ָ��
	cout << "\n�� IGA RM Shell ����ɹ����Ե��������£�\n";
	cout << "1.\t" << "���η���(��֧&�����غ�)\n";
	cout << "2.\t" << "Scordelis_Lo_roof �����غ�\n";
	cout << "3.\t" << "��ѹԲ��\n";
	cout << "4.\t" << "��ѹ����\n";
	cout << "5.\t" << "����þ��ΰ�����\n";
	cout << "6.\t" << "������������������֧���ΰ弯���غɣ�\n";
	cout << "7.\t" << "Patch Test\n";
	cout << "8.\t" << "ƽ������\n";
	cout << "\n������Ҫ���Ե�����(No.): ";
	cin >> testCase;
	cout << endl;
	cout << "������ H ϸ������(suggest:1-5): ";
	cin >> numHrefine;
	cout << endl;

	switch (testCase)
	{
	case 1:
	{
		// ��֧/��֧ ����/�����غ� ƽ��
		input = "../../TestCase/1-thin_square.xml";
		//bc_file = "../../TestCase/1-Rectangle_Plate_Simple_pressure.txt"; //��֧-�����غ�
		bc_file = "../../TestCase/1-thin_square_SIDE_simple.txt"; //��֧-�����غ�
		//bc_file = "../../TestCase/1-Rectangle_Plate_Fixed.txt"; //��֧-�����غ�
		m_material.thickness = 1.0e-3; // F=2.5��ԭʼ��
		m_material.E_modulus = 2e11;
		m_material.poisson_ratio = 0.3;
		// ��֧-�����غ� λ�Ʋο��� w = 0.0063336
	}
	break;
	case 2:
	{
		//Scordelis_Lo_roof �����غ�
		input = "../../TestCase/2-Scordelis_Lo_roof.xml";
		bc_file = "../../TestCase/2-Scordelis_Lo_roof.txt";
		m_material.thickness = 0.25;
		m_material.E_modulus = 4.32e8;
		m_material.poisson_ratio = 0.0;
		// λ�Ʋο��� w = -3.02E-01
	}
	break;
	case 3:
	{
		// ��ѹԲ��
		input = "../../TestCase/3-pinched_cylinder.xml";//1/4ģ��
		bc_file = "../../TestCase/3-pinched_cylinder.txt"; // 1/4
		m_material.thickness = 3.0E-2;
		m_material.E_modulus = 3e10;
		m_material.poisson_ratio = 0.3;
		// λ�Ʋο��� w = -1.82E-07
	}
	break;
	case 4:
	{
		// ��ѹ����
		input = "../../TestCase/4-Pinched_hemisphere.xml";	// 1/4
		bc_file = "../../TestCase/4-Pinched_hemisphere.txt"; //1/4
		m_material.thickness = 0.04;
		m_material.E_modulus = 6.825e7;
		m_material.poisson_ratio = 0.3;
		//λ�Ʋο��� x = 9.40E-02
	}
	break;
	case 5:
	{
		//����þ��ΰ�����
		input = "../../TestCase/5-Plate_square_L10.xml";	// 1/4
		bc_file = "../../TestCase/5-Plate_square_L10.txt"; //1/4
		m_material.thickness = 1.0;
		m_material.E_modulus = 1.0;
		m_material.poisson_ratio = 0.33;
		//λ�Ʋο��� v = -2.2152
	}
	break;
	case 6:
	{
		//��������������
		// ֣��ʦ��6���ɶ�PD�������е�ƽ������
		input = "../../TestCase/6-Rectangle_Plate.xml";
		bc_file = "../../TestCase/6-Rectangle_Plate_Simple.txt";
		m_material.thickness = 0.5;
		m_material.E_modulus = 2e5;
		m_material.poisson_ratio = 0.3;
		//λ�Ʋο���0.5687(��֧) ���֣��ʦMicro beam bond������
	}
	break;
	case 7:
	{
		// patch test
		input = "../../TestCase/7-patch_test.xml";
		bc_file = "../../TestCase/7-patch_test.txt";
		m_material.thickness = 1.0;
		m_material.E_modulus = 1000;
		m_material.poisson_ratio = 0.3;
		boolPatchTest = 1;
		// numHrefine = 0;	// ��ϸ��
	}
	break;
	case 8:
	{
		//��ʦ�ַ��������е�ƽ�����죬�������غ�
		input = "../../TestCase/8-square_stretch.xml";
		bc_file = "../../TestCase/8-square_stretch.txt";
		m_material.thickness = 1.0;
		m_material.E_modulus = 1.0;
		m_material.poisson_ratio = 0.33;
		//λ�Ʋο��� u = 10
	}
	break;
	case 9:
	{
		//���ն����Ƿ�ԳƵ�
		input = "../../TestCase/square_six.xml";
		bc_file = "../../TestCase/square_six.txt";
		m_material.thickness = 1.0;
		m_material.E_modulus = 1.0;
		m_material.poisson_ratio = 0.33;
		//λ�Ʋο��� u = 10
	}
	break;
	default:
		gsInfo << "Read data file ERROR!\n";
		break;
	}

	// �ж��ļ��Ƿ���ȷ����
	// �����ļ����������ʹ����� https://gismo.github.io/inputOutput_example.html
	if (!gsFileManager::fileExists(input))
	{
		gsWarn << "The file cannot be found!\n";
		return EXIT_FAILURE;
	}
	gsInfo << "\nRead file \"" << input << "\"\n";

	// �ж϶�����ļ����Ƿ��м���ģ��
	gsFileData<> fileData(input);
	gsGeometry<>::uPtr pGeom;
	if (fileData.has< gsGeometry<> >())
	{
		/* bool getFirst(Object & result)	const
		* Returns the first object of this type found in the XML data.
		* Doesn't look for nested objects. Writes it into the parameter.
		* https://gismo.github.io/classgismo_1_1gsFileData.html#a1d636af87cefa69eab119ac82b0d4632 */
		pGeom = fileData.getFirst< gsGeometry<> >();
	}
	else
	{
		gsWarn << "Input file doesn't have a geometry.\n";
		return EXIT_FAILURE;
	}
	gsInfo << "\nThe file contains: \n" << *pGeom << "\n";

	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	// 3.ǰ���� --------------------------------------------------------------------
	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	// 3.1 ǰ����--ģ��ϸ��
	gsMultiPatch<> multipatch;
	gsReadFile<> read_file(input, multipatch);
	// p-refine
	if (numElevate != 0)
	{
		multipatch.degreeElevate(numElevate);
	}
	// h-refine
	for (int r = 0; r < numHrefine; ++r)
	{
		multipatch.uniformRefine();
	}
	// ���ϸ�����ͼ��
	//gsWriteParaview(multipatch, "../../TestResult/multi_patch", 1000, true);
	gsInfo << "Export paraview files: ../../TestResult/multi_patch.pvd\n";

	// ϸ����Ļ�������Ϣ
	gsMultiBasis<> multibasis(multipatch);
	gsInfo << "\nPatches: " << multipatch.nPatches() << "\n";
	gsInfo << "basis.basis(0): " << multibasis.basis(0) << "\n";
	gsInfo << "Patch 0, knot vector xi: \n" << multibasis[0].component(0).detail() << "\n";
	gsInfo << "Patch 0, knot vector eta:\n" << multibasis[0].component(1).detail() << "\n";

	// Map ��Ϣ
	gsDofMapper dofmapper(multibasis, multipatch.nPatches());
	multibasis.getMapper(true, dofmapper, true);

	// 3.2 ��ȡ�ڵ�����,���Ƶ��������Ϣ
	gsNURBSinfo<real_t> nurbs_info(multipatch, multibasis, dofmapper);
	// gsNURBSinfo<real_t> nurbs_info(multipatch, multibasis); // ����1 ����

	// ����ģ��k�ļ�
	// ����1 ���ܲ���
	/*index_t nofp = 0;
	gsGeometry<real_t>& Geo = multipatch[nofp];
	nurbs_info.OutputKfile(Geo, "../../TestResult/RMShell.k");*/
	// ����2
	nurbs_info.ExportKfile("../../TestResult/RMShell.k");
	// ������k�ļ�����Ӳ��ϲ���������
	if (true)
	{
		// ��ʾָ��appģʽ����ֹ�������ݱ�����
		ofstream append("../../TestResult/RMShell.k", ofstream::app);
		// ������ϲ���
		append << "*MAT_ELASTIC\n";
		append << setw(10) << 1
			<< setw(10) << m_material.rho
			<< setw(10) << m_material.E_modulus
			<< setw(10) << m_material.poisson_ratio
			<< setw(10) << " " << setw(10) << " " << "\n";
		// ���part��Ϣ
		append << "*PART\n";
		append << "auto1\n";
		append << setw(10) << 1
			<< setw(10) << 1
			<< setw(10) << 1 << "\n";
		// ����ǵĺ��
		append << "*SECTION_SHELL\n";
		append << setw(10) << 1
			<< setw(10) << 0 << "\n";
		append << setw(10) << m_material.thickness
			<< setw(10) << 1.0 << setw(10) << 1.0 << setw(10) << 1.0 << "\n";
		append.close();
	}

	// 3.3 ����߽�����txt�ļ�
	if (!gsFileManager::fileExists(bc_file))
	{
		gsWarn << "The boundary file cannot be found!\n";
		return EXIT_FAILURE;
	}
	gsInfo << "\nRead boundary file \"" << bc_file << "\"\n";
	ifstream bc_stream;
	bc_stream.open(bc_file);

	// 3.4 ���߽��������ݴ���
	gsRMShellBoundary<real_t> BoundaryCondition(bc_stream,
		multibasis, nurbs_info, dof_node);

	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	// 4.��ֵ���� --------------------------------------------------------------------=
	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	// �ο� https://gismo.github.io/poisson_example.html

	// 4.1��������׼��--�ȼ�������
	gsRMShellAssembler<real_t> assembler(multipatch, multibasis, dofmapper,
		nurbs_info, m_material, BoundaryCondition, bc_stream, dof_node, memIntegPtAdd);

	// 4.1��������׼��--PD����
	PDIGACouple<double> pdAss(&assembler, nurbs_info, m_material);

	// Generate system matrix and load vector
	// 4.2װ��ն��� �غ���
	// 4.2.1 װ�� IGA �ն��� K
	gsInfo << "\nAssembling IGA K...";
	clock.restart();
	assembler.assemble();
	gsInfo << "done.\n";
	gsInfo << "=== >>> Time to assemble IGA global K : " << clock.stop() << "s.\n";
	gsInfo << "Have assembled a system (matrix and load vector) with "
		<< assembler.numDofs() * dof_node << " dofs.\n";

	// 4.2.2 װ�� PD �ն��� K_PD
	//װ��PD�ն���
	gsInfo << "\nAssembling PD K...";
	clock.restart();
	pdAss.ImplicitAnalysis();
	gsInfo << "done.\n";
	gsInfo << "=== >>> Time to assemble PD global K : " << clock.stop() << "s.\n";

	// 4.2.3 �ϳ���ϸն���
	gsInfo << "\nCoupling PD-IGA K... ";
	clock.restart();
	pdAss.combineStiffWithIGA();
	gsInfo << "done.\n";
	gsInfo << "=== >>> Time to Couple PD-IGA K : " << clock.stop() << "s.\n";

	// 4.2.4 ��ӱ߽�����
	gsInfo << "\nAdding boundary conditions... ";
	assembler.m_BoundaryCondition.setPressure(assembler.m_system);		// �����غ�
	assembler.m_BoundaryCondition.setpLoad(assembler.m_system);			// �����غ�
	assembler.m_BoundaryCondition.setDisp_constra(assembler.m_system);	// λ��Լ��
	gsInfo << "done.\n";

	// 4.2.5 ��Ƭ����
	if (boolPatchTest)
	{
		gsInfo << "\nPatch test ...";
		patch_test(assembler);
		gsInfo << "done.\n";
		return 0;
	}

	// 4.3���
	/* Initialize the conjugate gradient solver */
	// �����1 CG 
	/*gsInfo << "Solving...\n";
	gsSparseSolver<>::CGDiagonal solver(assembler.matrix());
	gsMatrix<> solVector = solver.solve(assembler.rhs());
	gsInfo << "Solved the system with CG solver.\n";*/

	// �����2 LU
	/* ��һ������� https://gismo.github.io/linearSolvers_example.html
	gsSparseSolver<>::LU solverLU;
	gsInfo << "\nEigen's LU: Started solving... ";
	clock.restart();
	solverLU.compute(assembler.matrix());
	gsMatrix<> solVector = solverLU.solve(assembler.rhs());
	gsInfo << "done.\n";
	gsInfo << "=== >>> Time to solve : " << clock.stop() << "s.\n";*/

	// �����3 Umf
	/*Eigen::SparseMatrix<double> eiK;
	assembler.gsSparseToSparse(eiK);*/
	gsMatrix<> solVector;
	solVector.setZero(assembler.rhs().rows(), 1);
	gsInfo << "\nUmf Solver: Started solving... ";
	clock.restart();
	gsumf_solver(assembler.matrix(), solVector, assembler.rhs());
	gsInfo << "done.\n";
	gsInfo << "=== >>> Time to solve : " << clock.stop() << "s.\n";

	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	// 5.������ --------------------------------------------------------------------=
	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	// 5.1 ��������
	// ���λ�ƽ� [u, v, w, thetax, thetay, thetaz]
	ofstream of_res(output.c_str());
	for (int i = 0; i < assembler.numDofs(); ++i)
	{
		of_res << setw(5) << i;
		for (int j = 0; j < 6; ++j)
		{
			of_res << setw(16) << solVector(i * 6 + j, 0);
		}
		of_res << endl;
	}
	of_res.close();

	// 5.2 ������κ��ģ��
	// Construct the solution as a scalar field
	gsMultiPatch<> mpsol; //gsMatrix<T>& coeffs = mpsol.patch(p).coefs();
	assembler.constructSolution(solVector, mpsol); // ��ȡ��λ��[u,v,w]��������ν��
	gsMultiPatch<> deformation = mpsol;
	for (index_t k = 0; k < multipatch.nPatches(); ++k)
	{
		deformation.patch(k).coefs() -= multipatch.patch(k).coefs();
	}
	gsField<> sol(mpsol, deformation);

	// 5.3 ���� -- ������ӻ�
	if (plot)
	{
		// 1. ����λ�ƽ�� Paraview ͼ��
		// Write approximate and exact solution to paraview files
		gsInfo << "\nPlotting in Paraview...\n";
		gsWriteParaview<>(sol, "../../TestResult/RM_Shell", 1000, true);
		gsInfo << "Export paraview files: \t.. / .. / TestResult / RM_Shell.pvd\n";
		// Run paraview
		//gsFileManager::open("../../TestResult/RM_Shell.pvd");

		// 2. ����λ��Ӧ��Ӧ���� HyperView ͼ��
		gsInfo << "\nPlotting in HyperView...\n";
		gsMatrix<> solMat; // [n x 6]
		solMat.setZero(nurbs_info.nurbs_sumcp, dof_node);
		assembler.vector2Matrix(solVector, solMat);
		// ����Ӧ��Ӧ��
		gsMatrix<> strainMat, stressMat;
		strainMat.setZero(nurbs_info.nurbs_sumcp, dof_node);
		stressMat.setZero(nurbs_info.nurbs_sumcp, dof_node);
		if (STRAINS)
		{
			assembler.StressStrain(solVector, strainMat, stressMat);
		}

		gsOutputHMRes("../../TestResult/RM_Shell.ascii",
			solMat, stressMat, strainMat);
		gsInfo << "Export HyperView files:\t../../TestResult/RM_Shell.ascii\n";
	}
	else
	{
		gsInfo << "Done. No output created, re-run with --plot"
			"to get a ParaView file containing the solution.\n";
	}

	// 5.3 ������ӻ�ģ��
	// �����ļ����������ʹ����� https://gismo.github.io/inputOutput_example.html
	if (output.empty())
	{
		gsInfo << "Call program with option -o <basename> to write data to files\n";
		gsInfo << "<basename>Paraview.vtp, <basename>Paraview.pvd, <basename>.xml\n";
		return EXIT_SUCCESS;
	}

	// ���Ƽ��� paraview ͼ��
	// writing a paraview file
	const std::string out = output + "Paraview";
	gsWriteParaview(*pGeom, out);
	gsInfo << "Export paraview files: \t" << out << ".vtp\n";
	gsInfo << "Export paraview files: \t" << out << ".pvd\n";

	/* ���ɼ���ģ�͵�xml�ļ�*/
	// writing a G+Smo .xml file
	gsFileData<> fd;
	fd << *pGeom;
	// output is a string. The extension .xml is added automatically
	fd.save(output);
	gsInfo << "Export G+Smo file:\t" << output << ".xml \n";


	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	// 6.������� --------------------------------------------------------------------
	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	gsInfo << "\nEnd of RM Shell program!\n";
	//return EXIT_SUCCESS;
	return 0;
}



