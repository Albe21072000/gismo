#pragma once
/** @file gsRMShellAssembler.h

   @brief Provides assembler for the RM shell equation.

   This file is part of the G+Smo library.

   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at http://mozilla.org/MPL/2.0/.

   Author(s): Y. Xia, HS. Wang

   Date:   2020-12-23
*/


#include <gsAssembler/gsAssembler.h>
#include "gsRMShellPDE.h"
#include "gsRMShellVisitor.hpp"
#include "gsRMShellBoundary.hpp"

namespace gismo
{
/** @brief
	Implementation of Reissner-Mindlin shell assembler.

	It sets up an assembler and assembles the system patch wise and combines
	the patch-local stiffness matrices into a global system by various methods
	(see gismo::gsInterfaceStrategy). It can also enforce Dirichlet boundary
	conditions in various ways (see gismo::gsDirichletStrategy).

	\ingroup Assembler
*/
template < class T>
class gsRMShellAssembler : public gsAssembler < T>
{
public:
	typedef gsAssembler<T> Base;

public:
	// Ĭ�Ϲ��캯��
	gsRMShellAssembler()
	{}

	/** @brief 
	Constructor of the assembler object.

	\param[in] patches is a gsMultiPatch object describing the geometry.
	\param[in] basis a multi-basis that contains patch-wise bases
	\param[in] bconditions is a gsBoundaryConditions object that holds all boundary conditions.
	\param[in] rhs is the right-hand side of the RM shell equation.
	\param[in] dirStrategy option for the treatment of Dirichlet boundary
	\param[in] intStrategy option for the treatment of patch interfaces

	\ingroup Assembler
	*/
	// ���캯��4
	gsRMShellAssembler(gsMultiPatch<T> const& patches,
		gsMultiBasis<T>			const& basis,
		gsDofMapper				const& dofmapper,
		gsNURBSinfo<T>			const& nurbs_data,
		Material& material,
		gsRMShellBoundary<T>& BoundaryCondition,
		ifstream& bc_stream,
		index_t					dof = 6,
		index_t					intept = 0)
		: m_patches(patches),
		m_basis(basis),
		m_dofmapper(dofmapper),
		m_nurbsinfo(nurbs_data),
		m_material(material),
		m_BoundaryCondition(BoundaryCondition)
	{
		m_dim = patches.parDim(); // �������ά��
		m_sum_nodes = basis.size();		// �ܿ��Ƶ���
		m_dofpn = dof;
		m_inte = intept;			// ���ӵĻ��ֵ���
		
		/* gismo���ڲ����� --- �߽���������*/
		// �ο� https://gismo.github.io/poisson_example.html
		dirichlet::strategy     dirStrategy = dirichlet::none;	// ��Ȼû�ã�����Ҫ��
			iFace::strategy         intStrategy = iFace::none;	// ��Ȼû�ã�����Ҫ��
		m_options.setInt("DirichletStrategy", dirStrategy);
		m_options.setInt("InterfaceStrategy", intStrategy);

		gsBoundaryConditions<> bcInfo;	// ��Ȼû�ã�����Ҫ��
		gsFunctionExpr<> rhs;			// ��Ȼû�ã�����Ҫ��
		typename gsPde<T>::Ptr pde(new gsRMShellPde<T>(m_patches, bcInfo, rhs));

		Base::initialize(pde, m_basis, m_options);
		
		// ����߽�������Ϣ��Լ�����غ�
		m_BoundaryCondition.setBoundary(bc_stream);
	}

	~gsRMShellAssembler()
	{}
	
	// Refresh routine
	void refresh();
	
	// Main assembly routine
	void assemble();
	
	void constructSolution(gsMatrix<T>& solVector,
		gsMultiPatch<T>& result);

	virtual	void StressStrain(const gsMatrix<T>& solVector,
		gsMatrix<T>& StrainVec, gsMatrix<T>& StressVec);

	// �����ת�� 
	virtual void vector2Matrix(const gsMatrix<T>& solVector,
		gsMatrix<T>& result);

	virtual void VectorgoMatrix(const gsVector<T>& in_vector,
		gsMatrix<T>& result);

	virtual void gsMatrix2Matrixxf(const gsMatrix<T>& a,
		Eigen::MatrixXf& aCopy);

	virtual void gsSparseToSparse(Eigen::SparseMatrix<T>& eiK);

	virtual void gsMatrixtoVector(vector<T>& rhs_v);

	Eigen::SparseSelfAdjointView< typename gsSparseMatrix<T>::Base, Lower> fullMatrix()
	{
	     return m_system.matrix().template selfadjointView<Lower>();
	}

public:
	Material			m_material;				// ���ϲ���
	index_t				m_sum_nodes;			// ���Ƶ�����
	index_t				m_dofpn;				// �ڵ����ɶ� = 6
	index_t				m_inte;					// ���ӵĻ��ֵ����
	index_t				m_dim;					// �������ά�� = 2 [xi eta]
	ifstream			m_bcfile;				// �߽�������txt�ļ�
	gsMultiPatch<T>		m_patches;				// patch ��Ϣ
	gsMultiBasis<T>		m_basis;				// ��������Ϣ
	gsDofMapper			m_dofmapper;			// ��Ƭ��ӳ����Ϣ
	gsNURBSinfo<T>		m_nurbsinfo;			// nurbs ��Ϣ
	gsRMShellBoundary<T> m_BoundaryCondition;	// �߽���������
	SSdata<T>			m_SSdata;

public:
	// ģ�Ͳ�����Ϣ
	set<real_t> m_tempSet1;
	set<real_t> m_tempSet2;
	/* the knot vectors in the patches, 
	* inner vector<double> has the knot vector of one patch,
	* multiple same values will be reduced to only one value */
	vector<vector<double> > m_U_knotVectors_Patches;  
	vector<vector<double> > m_V_knotVectors_Patches;
	/* all the control points in the multiple patch, 
	* each row has one control point, stored in sequence of dof mapper;	*/
	gsMatrix<T> m_AllCps; 
	// ���п��Ƶ��Ӧ�Ĳ���������꣬Greville����
	gsMatrix<T> m_AllParaps;
	
public:

	// Members from gsAssembler
	using Base::m_pde_ptr;
	using Base::m_bases;
	using Base::m_ddof;
	using Base::m_options;
	using Base::m_system;						// �����ն���K���غ���rhs

 }; // class gsRMShellAssembler 

} // namespace gismo


#ifndef GISMO_BUILD_LIB
#include GISMO_HPP_HEADER(gsRMShellAssembler.hpp)
#endif