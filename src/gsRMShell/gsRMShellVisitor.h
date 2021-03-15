#pragma once
/** @file gsRMShellVisitor.h

    @brief RM shell element visitor.

    This file is part of the G+Smo library.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.

    Author(s): Y. Xia, HS. Wang

    Date:   2020-12-23
*/

#include <gsAssembler/gsQuadrature.h>
#include "gsGeometryEvaluator.hpp"
#include "gsRMShellBoundary.hpp"
#include "gsRMShellBase.h"

namespace gismo
{

    /** \brief Visitor for the RM shell equation.
     *
     * Assembles the bilinear terms
     */

    template <class T>
    class gsRMShellVisitor
    {
    public:

        /** \brief Constructor for gsRMShellVisitor.
         */
        gsRMShellVisitor(const gsPde<T>& pde)
        {
            pde_ptr = static_cast<const gsPoissonPde<T>*>(&pde);
        }
    public:
        // ���캯��
        gsRMShellVisitor(gsMultiPatch<T> const& patches,
            gsDofMapper         const& dofmapper,
            Material            material,
            gsDistriLoads<T>& pressure_,
            SSdata<T>& ssData,
            int                 dofs,
            int                 ptInt = 0) :
            multi_patch(patches),
            m_dofmap(dofmapper),
            m_material(material),
            m_pressure(pressure_),
            m_ssdata(ssData),
            dof(dofs),
            m_inte(ptInt)
        {
            rhs_cols = m_pressure.numLoads();
            computeDMatrix();
        }

        // ���ƹ��캯��
        /*gsRMShellVisitor(gsRMShellVisitor<T>& C)
        {
            multi_patch = C.multi_patch;
            m_material = C.m_material;
            m_pressure = C.m_pressure;
            dof = C.dof;
            m_inte = C.m_inte;
            rhs_cols = C.rhs_cols;
        }*/

        ~gsRMShellVisitor()
        {

        }

        // 0.���㵯�Ծ��� D
        inline void computeDMatrix();


        // 1.�趨���ֹ���
        void initialize(const gsBasis<T>& basis,
            const index_t       patchIndex,
            const gsOptionList& options,
            gsQuadRule<T>& rule);


        // 2.1.compute greville abscissa points
        inline void compute_greville();


        // 2.2.����greville��ķ�����
        inline void comput_norm(gsGenericGeometryEvaluator< T, 2, 1 >& geoEval,
            const gsMatrix<T>& Nodes,
            gsMatrix<T>& norm);


        // 2.3.��������ת������ T
        inline void comput_gauss_tnb(gsMatrix<T> const& quNodes);

        // 2.Evaluate on element.
        inline void evaluate(const gsBasis<T>& basis,
            const gsGeometry<T>& geo,
            const gsMatrix<T>& quNodes);


        // 3.1.�����ſɱȾ���
        inline void computeJacob(gsMatrix<T>& bGrads,
            gsMatrix<T>& bVals,
            gsMatrix<T>& J,
            const real_t z,
            const index_t k);

        /// 3.2.Computes the membrane and flexural strain
        //  first derivatives at greville point \em k
        inline void computeStrainB(gsMatrix<T>& bVals,
            const gsMatrix<T>& bGrads,
            const real_t  t,
            const index_t k);

        // 3.3.ȫ�ֵ�����
        inline void comput_D_global(int const k,
            gsMatrix<T>& D_global);

        // 3.4.�����ֲ��նȾ���Ĳ�����ʹ�������
        inline void fixSingularity();

        // 3.5.�����غɼ���׼��
        inline void comput_rhs_press(gsDistriLoads<T>& m_pressure,
            gsMatrix<T>& Jacob,
            gsMatrix<T>& bVals,
            index_t dots,
            index_t k,
            index_t rr);

        // 3.�㵥��
        inline void assemble(gsDomainIterator<T>&,
            gsVector<T> const& quWeights);


        // 4.���ܸ�
        inline void localToGlobal(const index_t patchIndex,
            const std::vector<gsMatrix<T> >& eliminatedDofs,
            gsSparseSystem<T>& system);


    public:
        index_t          dof;           // �ڵ����ɶ�
        index_t          m_patchcount;  // Ƭ��
        index_t          m_inte;        // ���ӻ��ֵ����
        Material         m_material;    // ���ϲ���
        gsMatrix<real_t> globalCp;      // ȫ�����Ƶ�����
        gsMatrix<real_t> localCp;       // ��Ԫ��Ӧ�Ŀ��Ƶ�����
        gsMatrix<real_t> greville_pt;   // ����ά����
        gsMatrix<real_t> greville_n;    // ����ά���㴦�ķ�����
        gsMatrix<real_t> m_t1;          // ����ת������ t n b
        gsMatrix<real_t> m_t2;          // ����ת������ t n b
        gsMatrix<real_t> m_Gn;          // ����ת������ t n b
        gsVector<real_t> m_normal;      // ����ά���㴦�ķ�����
        gsMultiPatch<T>  multi_patch;
        gsDofMapper      m_dofmap;
        gsDistriLoads<T> m_pressure;    // �����غ���Ϣ��ʵΪ�ֲ�������

    public:
        gsMatrix<real_t> localD;        // �ֲ�����ϵ�µ�D
        gsMatrix<real_t> globalD;       // ȫ������ϵ�µ�D
        gsMatrix<real_t> B_aW;          // Ӧ�����B
        gsMatrix<real_t> B_tW;          // Ӧ�����B
        gsMatrix<real_t> globalB;       // ��Ԫ��B��
        gsMatrix<real_t> Jacob;         // �ſɱȾ���

        // �洢����������ݣ��Ժ����Ӧ��Ӧ�䣬PS: SS Ϊ Stress Strain ����д
        SSdata<T>& m_ssdata;
        gsMatrix<real_t> SSDl;          // ���ڼ���Ӧ��Ӧ��ĵ�Ԫ�ϵ�D
        gsMatrix<real_t> SSBl;          // ���ڼ���Ӧ��Ӧ��ĵ�Ԫ�ϵ�B
        gsMatrix<real_t> SSNl;          // ���ڼ���Ӧ��Ӧ��ĵ�Ԫ�ϵĻ�����N
        gsMatrix<index_t> SSPl;         // ���ڼ���Ӧ��Ӧ��ĵ�Ԫ�ϵĿ��Ƶ���P
    protected:
        // Pointer to the pde data
        const gsPoissonPde<T>* pde_ptr;

    protected:
        // Basis values
        std::vector<gsMatrix<T> >   basisData;  // ���ֵ�Ļ�����ֵ�ͻ���������ֵ
        std::vector < vector<gsMatrix<T> > >  basisData_p;// ���ֵ�Ļ�����ֵ�ͻ���������ֵ(�����غ�)
        gsMatrix<T>                 physGrad;   // û���õ�
        gsMatrix<index_t>           actives;    // ��Ԫ��Ӧ�Ŀ��Ƶ���
        gsMatrix<index_t>           gloActives; // ��Ԫ��Ӧ�Ŀ��Ƶ��ŵ�ȫ�ֱ��
        index_t                     numActive;  // ��Ԫ��Ӧ�Ŀ��Ƶ�����
        index_t                     rhs_cols;   // �����غ���
    protected:
        // Right hand side ptr for current patch
        const gsFunction<T>* rhs_ptr;    // û���õ�

        // Local values of the right hand side
        gsMatrix<T>                 rhsVals;    // ���ڻ�����ֵx������������غ�׼�����ݣ�

    protected:
        // Local matrices
        gsMatrix<T>                 localMat;   // ��Ԫ�ն���
        gsMatrix<T>                 localRhs;   // ��Ԫ�غ���

        gsMapData<T>                map_data;   // ���ֹ�����Ϣ
    };


} // namespace gismo


