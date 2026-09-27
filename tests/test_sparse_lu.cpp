#include "test_harness.hpp"
#include <bharatopt/sparse_lu.hpp>
#include <bharatopt/revised_simplex.hpp>
#include <bharatopt/presolve.hpp>

using namespace bharatopt;

TEST_CASE(SparseLU_01_IdentityMatrix) {
    COOMatrix coo(3, 3);
    coo.add_entry(0, 0, 1.0);
    coo.add_entry(1, 1, 1.0);
    coo.add_entry(2, 2, 1.0);
    CSRMatrix B = CSRMatrix::from_coo(coo);

    SparseLUBasisSolver solver;
    EXPECT_TRUE(solver.factorize_matrix(B));

    std::vector<real_t> rhs = {2.0, 5.0, 7.0};
    std::vector<real_t> x, y;
    EXPECT_TRUE(solver.solve_primal(rhs, x));
    EXPECT_TRUE(solver.solve_dual(rhs, y));

    EXPECT_NEAR(x[0], 2.0, 1e-12);
    EXPECT_NEAR(x[1], 5.0, 1e-12);
    EXPECT_NEAR(x[2], 7.0, 1e-12);

    EXPECT_NEAR(y[0], 2.0, 1e-12);
    EXPECT_NEAR(y[1], 5.0, 1e-12);
    EXPECT_NEAR(y[2], 7.0, 1e-12);
}

TEST_CASE(SparseLU_02_DiagonalMatrix) {
    COOMatrix coo(3, 3);
    coo.add_entry(0, 0, 2.0);
    coo.add_entry(1, 1, 4.0);
    coo.add_entry(2, 2, 5.0);
    CSRMatrix B = CSRMatrix::from_coo(coo);

    SparseLUBasisSolver solver;
    EXPECT_TRUE(solver.factorize_matrix(B));

    std::vector<real_t> rhs = {10.0, 20.0, 35.0};
    std::vector<real_t> x;
    EXPECT_TRUE(solver.solve_primal(rhs, x));

    EXPECT_NEAR(x[0], 5.0, 1e-12);
    EXPECT_NEAR(x[1], 5.0, 1e-12);
    EXPECT_NEAR(x[2], 7.0, 1e-12);
}

TEST_CASE(SparseLU_03_SmallDenseMatrix) {
    COOMatrix coo(3, 3);
    // [ 2  1  1 ]
    // [ 1  2  0 ]
    // [ 0  1  2 ]
    coo.add_entry(0, 0, 2.0); coo.add_entry(0, 1, 1.0); coo.add_entry(0, 2, 1.0);
    coo.add_entry(1, 0, 1.0); coo.add_entry(1, 1, 2.0);
    coo.add_entry(2, 1, 1.0); coo.add_entry(2, 2, 2.0);
    CSRMatrix B = CSRMatrix::from_coo(coo);

    SparseLUBasisSolver solver;
    EXPECT_TRUE(solver.factorize_matrix(B));

    std::vector<real_t> rhs = {8.0, 8.0, 8.0};
    std::vector<real_t> x;
    EXPECT_TRUE(solver.solve_primal(rhs, x));

    // B * x should equal rhs
    std::vector<real_t> res(3, 0.0);
    B.multiply(x.data(), res.data());
    for (size_t i = 0; i < 3; ++i) {
        EXPECT_NEAR(res[i], rhs[i], 1e-7);
    }
}

TEST_CASE(SparseLU_04_SparseMatrix) {
    COOMatrix coo(10, 10);
    for (int i = 0; i < 10; ++i) {
        coo.add_entry(i, i, static_cast<real_t>(i + 1));
        if (i > 0) coo.add_entry(i, i - 1, 0.5);
    }
    CSRMatrix B = CSRMatrix::from_coo(coo);

    SparseLUBasisSolver solver;
    EXPECT_TRUE(solver.factorize_matrix(B));

    std::vector<real_t> rhs(10, 1.0);
    std::vector<real_t> x;
    EXPECT_TRUE(solver.solve_primal(rhs, x));

    std::vector<real_t> res(10, 0.0);
    B.multiply(x.data(), res.data());
    for (size_t i = 0; i < 10; ++i) {
        EXPECT_NEAR(res[i], 1.0, 1e-7);
    }
}

TEST_CASE(SparseLU_05_PermutedSparseMatrix) {
    COOMatrix coo(3, 3);
    // Row 0 has 0 in diagonal, requiring pivot swap
    coo.add_entry(0, 1, 2.0);
    coo.add_entry(1, 0, 4.0);
    coo.add_entry(2, 2, 3.0);
    CSRMatrix B = CSRMatrix::from_coo(coo);

    SparseLUBasisSolver solver;
    EXPECT_TRUE(solver.factorize_matrix(B));

    std::vector<real_t> rhs = {6.0, 8.0, 9.0};
    std::vector<real_t> x;
    EXPECT_TRUE(solver.solve_primal(rhs, x));

    EXPECT_NEAR(x[0], 2.0, 1e-12);
    EXPECT_NEAR(x[1], 3.0, 1e-12);
    EXPECT_NEAR(x[2], 3.0, 1e-12);
}

TEST_CASE(SparseLU_06_TriangularMatrix) {
    COOMatrix coo(3, 3);
    coo.add_entry(0, 0, 1.0);
    coo.add_entry(1, 0, 2.0); coo.add_entry(1, 1, 3.0);
    coo.add_entry(2, 0, 4.0); coo.add_entry(2, 1, 5.0); coo.add_entry(2, 2, 6.0);
    CSRMatrix B = CSRMatrix::from_coo(coo);

    SparseLUBasisSolver solver;
    EXPECT_TRUE(solver.factorize_matrix(B));

    std::vector<real_t> rhs = {1.0, 5.0, 20.0};
    std::vector<real_t> x;
    EXPECT_TRUE(solver.solve_primal(rhs, x));

    std::vector<real_t> res(3, 0.0);
    B.multiply(x.data(), res.data());
    for (size_t i = 0; i < 3; ++i) {
        EXPECT_NEAR(res[i], rhs[i], 1e-7);
    }
}

TEST_CASE(SparseLU_07_SingularMatrix) {
    COOMatrix coo(3, 3);
    coo.add_entry(0, 0, 1.0);
    coo.add_entry(1, 1, 2.0);
    // Row 2 is completely zero -> Singular!
    CSRMatrix B = CSRMatrix::from_coo(coo);

    SparseLUBasisSolver solver;
    EXPECT_FALSE(solver.factorize_matrix(B));
}

TEST_CASE(SparseLU_08_NearSingularMatrix) {
    COOMatrix coo(2, 2);
    coo.add_entry(0, 0, 1e-12);
    coo.add_entry(1, 1, 1e-12);
    CSRMatrix B = CSRMatrix::from_coo(coo);

    SparseLUBasisSolver solver(1e-10); // Pivot tolerance 1e-10
    EXPECT_FALSE(solver.factorize_matrix(B));
}

TEST_CASE(SparseLU_09_ZeroPivot) {
    COOMatrix coo(2, 2);
    coo.add_entry(0, 1, 5.0);
    coo.add_entry(1, 0, 5.0);
    CSRMatrix B = CSRMatrix::from_coo(coo);

    SparseLUBasisSolver solver;
    EXPECT_TRUE(solver.factorize_matrix(B)); // Handled by row permutation
}

TEST_CASE(SparseLU_10_NearZeroPivot) {
    COOMatrix coo(2, 2);
    coo.add_entry(0, 0, 1e-11);
    coo.add_entry(0, 1, 1.0);
    coo.add_entry(1, 0, 1.0);
    coo.add_entry(1, 1, 1.0);
    CSRMatrix B = CSRMatrix::from_coo(coo);

    SparseLUBasisSolver solver;
    EXPECT_TRUE(solver.factorize_matrix(B)); // Row 1 swapped with Row 0
}

TEST_CASE(SparseLU_11_ForwardSolve) {
    COOMatrix coo(3, 3);
    coo.add_entry(0, 0, 2.0);
    coo.add_entry(1, 1, 3.0);
    coo.add_entry(2, 2, 4.0);
    CSRMatrix B = CSRMatrix::from_coo(coo);

    SparseLUBasisSolver solver;
    EXPECT_TRUE(solver.factorize_matrix(B));

    std::vector<real_t> rhs = {4.0, 9.0, 16.0};
    std::vector<real_t> x;
    EXPECT_TRUE(solver.solve_primal(rhs, x));

    EXPECT_NEAR(x[0], 2.0, 1e-12);
    EXPECT_NEAR(x[1], 3.0, 1e-12);
    EXPECT_NEAR(x[2], 4.0, 1e-12);
}

TEST_CASE(SparseLU_12_BackwardSolve) {
    COOMatrix coo(3, 3);
    coo.add_entry(0, 0, 1.0); coo.add_entry(0, 1, 2.0); coo.add_entry(0, 2, 3.0);
    coo.add_entry(1, 1, 4.0); coo.add_entry(1, 2, 5.0);
    coo.add_entry(2, 2, 6.0);
    CSRMatrix B = CSRMatrix::from_coo(coo);

    SparseLUBasisSolver solver;
    EXPECT_TRUE(solver.factorize_matrix(B));

    std::vector<real_t> rhs = {6.0, 9.0, 6.0};
    std::vector<real_t> x;
    EXPECT_TRUE(solver.solve_primal(rhs, x));

    EXPECT_NEAR(x[0], 1.0, 1e-12);
    EXPECT_NEAR(x[1], 1.0, 1e-12);
    EXPECT_NEAR(x[2], 1.0, 1e-12);
}

TEST_CASE(SparseLU_13_TransposeSolve) {
    COOMatrix coo(3, 3);
    // B = [ 2  1  0 ]
    //     [ 0  3  1 ]
    //     [ 1  0  4 ]
    coo.add_entry(0, 0, 2.0); coo.add_entry(0, 1, 1.0);
    coo.add_entry(1, 1, 3.0); coo.add_entry(1, 2, 1.0);
    coo.add_entry(2, 0, 1.0); coo.add_entry(2, 2, 4.0);
    CSRMatrix B = CSRMatrix::from_coo(coo);

    SparseLUBasisSolver solver;
    EXPECT_TRUE(solver.factorize_matrix(B));

    std::vector<real_t> rhs = {5.0, 7.0, 9.0};
    std::vector<real_t> y;
    EXPECT_TRUE(solver.solve_dual(rhs, y));

    // Verify B^T * y == rhs
    // B^T = [ 2  0  1 ]
    //       [ 1  3  0 ]
    //       [ 0  1  4 ]
    real_t r0 = 2.0 * y[0] + 0.0 * y[1] + 1.0 * y[2];
    real_t r1 = 1.0 * y[0] + 3.0 * y[1] + 0.0 * y[2];
    real_t r2 = 0.0 * y[0] + 1.0 * y[1] + 4.0 * y[2];

    EXPECT_NEAR(r0, 5.0, 1e-7);
    EXPECT_NEAR(r1, 7.0, 1e-7);
    EXPECT_NEAR(r2, 9.0, 1e-7);
}

TEST_CASE(SparseLU_14_PermutationCorrectness) {
    COOMatrix coo(3, 3);
    coo.add_entry(0, 2, 3.0);
    coo.add_entry(1, 0, 1.0);
    coo.add_entry(2, 1, 2.0);
    CSRMatrix B = CSRMatrix::from_coo(coo);

    SparseLUBasisSolver solver;
    EXPECT_TRUE(solver.factorize_matrix(B));

    std::vector<real_t> rhs = {9.0, 2.0, 8.0};
    std::vector<real_t> x;
    EXPECT_TRUE(solver.solve_primal(rhs, x));

    EXPECT_NEAR(x[0], 2.0, 1e-12);
    EXPECT_NEAR(x[1], 4.0, 1e-12);
    EXPECT_NEAR(x[2], 3.0, 1e-12);
}

TEST_CASE(SparseLU_15_FillInAccounting) {
    COOMatrix coo(3, 3);
    coo.add_entry(0, 0, 2.0); coo.add_entry(0, 1, 1.0);
    coo.add_entry(1, 0, 1.0); coo.add_entry(1, 1, 2.0); coo.add_entry(1, 2, 1.0);
    coo.add_entry(2, 1, 1.0); coo.add_entry(2, 2, 2.0);
    CSRMatrix B = CSRMatrix::from_coo(coo);

    SparseLUBasisSolver solver;
    EXPECT_TRUE(solver.factorize_matrix(B));

    const auto& stats = solver.stats();
    EXPECT_EQ(stats.matrix_dim, static_cast<size_t>(3));
    EXPECT_EQ(stats.original_basis_nnz, static_cast<size_t>(7));
    EXPECT_TRUE(stats.total_factor_nnz >= 7);
    EXPECT_TRUE(stats.fill_in_ratio >= 1.0);
}

TEST_CASE(SparseLU_16_BasisExtraction) {
    LPModel model("basis_extract");
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 1.0);
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 2.0);

    model.add_constraint("c1", {{x1, 2.0}, {x2, 1.0}}, ConstraintSense::LESS_EQUAL, 8.0);
    model.add_constraint("c2", {{x1, 1.0}, {x2, 2.0}}, ConstraintSense::LESS_EQUAL, 8.0);

    RevisedSimplex rsolver;
    StandardFormLP std_lp = rsolver.create_standard_form(model);

    Basis basis(2, 4);
    basis.set_initial_basis({2, 3}); // Slacks in basis

    CSRMatrix B_extracted = SparseLUBasisSolver::extract_sparse_basis(std_lp, basis);
    EXPECT_EQ(B_extracted.rows(), static_cast<size_t>(2));
    EXPECT_EQ(B_extracted.cols(), static_cast<size_t>(2));
    EXPECT_EQ(B_extracted.nnz(), static_cast<size_t>(2)); // Identity matrix 2x2
}

TEST_CASE(SparseLU_17_DenseVsSparseSolutionComparison) {
    COOMatrix coo(4, 4);
    coo.add_entry(0, 0, 4.0); coo.add_entry(0, 1, 1.0);
    coo.add_entry(1, 0, 1.0); coo.add_entry(1, 1, 5.0); coo.add_entry(1, 2, 2.0);
    coo.add_entry(2, 1, 2.0); coo.add_entry(2, 2, 6.0); coo.add_entry(2, 3, 1.0);
    coo.add_entry(3, 2, 1.0); coo.add_entry(3, 3, 3.0);
    CSRMatrix B = CSRMatrix::from_coo(coo);

    SparseLUBasisSolver sparse_solver;
    EXPECT_TRUE(sparse_solver.factorize_matrix(B));

    std::vector<real_t> rhs = {10.0, 15.0, 20.0, 12.0};
    std::vector<real_t> x_sparse, y_sparse;
    EXPECT_TRUE(sparse_solver.solve_primal(rhs, x_sparse));
    EXPECT_TRUE(sparse_solver.solve_dual(rhs, y_sparse));

    // Dense solver comparison setup
    StandardFormLP lp;
    lp.num_rows = 4;
    lp.num_cols = 4;
    lp.A.assign(4, std::vector<real_t>(4, 0.0));
    for (size_t i = 0; i < 4; ++i) {
        size_t start = static_cast<size_t>(B.row_offsets()[i]);
        size_t end = static_cast<size_t>(B.row_offsets()[i+1]);
        for (size_t p = start; p < end; ++p) {
            lp.A[i][B.col_indices()[p]] = B.values()[p];
        }
    }
    lp.b = rhs;
    lp.c = rhs;

    Basis basis(4, 4);
    basis.set_initial_basis({0, 1, 2, 3});

    DenseBasisSolver dense_solver;
    EXPECT_TRUE(dense_solver.factorize(lp, basis));

    std::vector<real_t> x_dense, y_dense;
    EXPECT_TRUE(dense_solver.solve_primal(rhs, x_dense));
    EXPECT_TRUE(dense_solver.solve_dual(rhs, y_dense));

    for (size_t i = 0; i < 4; ++i) {
        EXPECT_NEAR(x_sparse[i], x_dense[i], 1e-7);
        EXPECT_NEAR(y_sparse[i], y_dense[i], 1e-7);
    }
}

TEST_CASE(SparseLU_18_MultipleRHSSolves) {
    COOMatrix coo(3, 3);
    coo.add_entry(0, 0, 3.0); coo.add_entry(0, 1, 1.0);
    coo.add_entry(1, 1, 4.0); coo.add_entry(1, 2, 2.0);
    coo.add_entry(2, 0, 1.0); coo.add_entry(2, 2, 5.0);
    CSRMatrix B = CSRMatrix::from_coo(coo);

    SparseLUBasisSolver solver;
    EXPECT_TRUE(solver.factorize_matrix(B));

    std::vector<std::vector<real_t>> rhss = {
        {1.0, 2.0, 3.0},
        {10.0, 20.0, 30.0},
        {-5.0, 15.0, 0.0},
        {0.0, 0.0, 0.0},
        {7.0, 14.0, 21.0}
    };

    for (const auto& rhs : rhss) {
        std::vector<real_t> x;
        EXPECT_TRUE(solver.solve_primal(rhs, x));
        std::vector<real_t> res(3, 0.0);
        B.multiply(x.data(), res.data());
        for (size_t i = 0; i < 3; ++i) {
            EXPECT_NEAR(res[i], rhs[i], 1e-7);
        }
    }
}

TEST_CASE(SparseLU_19_DifferentSparsityPatterns) {
    // Arrow-head matrix
    COOMatrix coo(4, 4);
    coo.add_entry(0, 0, 10.0); coo.add_entry(0, 1, 1.0); coo.add_entry(0, 2, 1.0); coo.add_entry(0, 3, 1.0);
    coo.add_entry(1, 0, 1.0); coo.add_entry(1, 1, 5.0);
    coo.add_entry(2, 0, 1.0); coo.add_entry(2, 2, 5.0);
    coo.add_entry(3, 0, 1.0); coo.add_entry(3, 3, 5.0);
    CSRMatrix B = CSRMatrix::from_coo(coo);

    SparseLUBasisSolver solver;
    EXPECT_TRUE(solver.factorize_matrix(B));

    std::vector<real_t> rhs = {13.0, 6.0, 6.0, 6.0};
    std::vector<real_t> x;
    EXPECT_TRUE(solver.solve_primal(rhs, x));

    EXPECT_NEAR(x[0], 1.0, 1e-7);
    EXPECT_NEAR(x[1], 1.0, 1e-7);
    EXPECT_NEAR(x[2], 1.0, 1e-7);
    EXPECT_NEAR(x[3], 1.0, 1e-7);
}

TEST_CASE(SparseLU_20_NumericalScaling) {
    COOMatrix coo(3, 3);
    coo.add_entry(0, 0, 1e-4); coo.add_entry(0, 1, 1.0);
    coo.add_entry(1, 1, 1e4);  coo.add_entry(1, 2, 1.0);
    coo.add_entry(2, 0, 1.0);  coo.add_entry(2, 2, 1e-4);
    CSRMatrix B = CSRMatrix::from_coo(coo);

    SparseLUBasisSolver solver;
    EXPECT_TRUE(solver.factorize_matrix(B));

    std::vector<real_t> rhs = {1.0, 10000.0, 1.0};
    std::vector<real_t> x;
    EXPECT_TRUE(solver.solve_primal(rhs, x));

    std::vector<real_t> res(3, 0.0);
    B.multiply(x.data(), res.data());
    for (size_t i = 0; i < 3; ++i) {
        EXPECT_NEAR(res[i], rhs[i], 1e-3);
    }
}

TEST_CASE(SparseLU_21_RevisedSimplexDenseSolver) {
    LPModel model("dense_solver_test");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 3.0);
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 5.0);

    model.add_constraint("c1", {{x1, 2.0}, {x2, 1.0}}, ConstraintSense::LESS_EQUAL, 8.0);
    model.add_constraint("c2", {{x1, 1.0}, {x2, 2.0}}, ConstraintSense::LESS_EQUAL, 8.0);

    RevisedSimplexOptions opts;
    opts.solver_type = BasisSolverType::DENSE_LU;
    RevisedSimplex solver(opts);
    RevisedSimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, RevisedSimplexStatus::OPTIMAL);
    EXPECT_NEAR(res.objective_value, 64.0 / 3.0, 1e-6);
}

TEST_CASE(SparseLU_22_RevisedSimplexSparseLUSolver) {
    LPModel model("sparse_lu_solver_test");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 3.0);
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 5.0);

    model.add_constraint("c1", {{x1, 2.0}, {x2, 1.0}}, ConstraintSense::LESS_EQUAL, 8.0);
    model.add_constraint("c2", {{x1, 1.0}, {x2, 2.0}}, ConstraintSense::LESS_EQUAL, 8.0);

    RevisedSimplexOptions opts;
    opts.solver_type = BasisSolverType::SPARSE_LU;
    RevisedSimplex solver(opts);
    RevisedSimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, RevisedSimplexStatus::OPTIMAL);
    EXPECT_NEAR(res.objective_value, 64.0 / 3.0, 1e-6);
}

TEST_CASE(SparseLU_23_BothSolversEquivalence) {
    LPModel model("both_solvers_equivalence");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 10.0);
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 6.0);
    index_t x3 = model.add_variable("x3", 0.0, BHARATOPT_INFINITY, 4.0);

    model.add_constraint("c1", {{x1, 1.0}, {x2, 1.0}, {x3, 1.0}}, ConstraintSense::LESS_EQUAL, 100.0);
    model.add_constraint("c2", {{x1, 10.0}, {x2, 4.0}, {x3, 5.0}}, ConstraintSense::LESS_EQUAL, 600.0);
    model.add_constraint("c3", {{x1, 2.0}, {x2, 2.0}, {x3, 6.0}}, ConstraintSense::LESS_EQUAL, 300.0);

    RevisedSimplexOptions opts_dense;
    opts_dense.solver_type = BasisSolverType::DENSE_LU;
    RevisedSimplex solver_dense(opts_dense);
    RevisedSimplexResult res_dense = solver_dense.solve(model);

    RevisedSimplexOptions opts_sparse;
    opts_sparse.solver_type = BasisSolverType::SPARSE_LU;
    RevisedSimplex solver_sparse(opts_sparse);
    RevisedSimplexResult res_sparse = solver_sparse.solve(model);

    EXPECT_EQ(res_dense.status, RevisedSimplexStatus::OPTIMAL);
    EXPECT_EQ(res_sparse.status, RevisedSimplexStatus::OPTIMAL);
    EXPECT_NEAR(res_dense.objective_value, res_sparse.objective_value, 1e-6);

    for (size_t j = 0; j < model.num_variables(); ++j) {
        EXPECT_NEAR(res_dense.primal_solution[j], res_sparse.primal_solution[j], 1e-6);
    }
}

TEST_CASE(SparseLU_24_PresolveRevisedSparsePostsolvePipeline) {
    LPModel model("pipeline_sparse_test");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x1 = model.add_variable("x1", 4.0, 4.0, 2.0); // Fixed x1 = 4
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 3.0);
    index_t x3 = model.add_variable("x3", 0.0, BHARATOPT_INFINITY, 5.0);

    model.add_constraint("c1", {{x1, 2.0}, {x2, 1.0}, {x3, 2.0}}, ConstraintSense::LESS_EQUAL, 20.0);
    model.add_constraint("c2", {{x1, 1.0}, {x2, 2.0}, {x3, 1.0}}, ConstraintSense::LESS_EQUAL, 16.0);

    // Step 1: Run Presolve
    PresolveEngine presolver;
    PresolveResult presolve_res = presolver.presolve(model);
    EXPECT_EQ(presolve_res.status, PresolveStatus::SUCCESS);

    // Step 2: Solve Reduced Model with Revised Simplex (SPARSE_LU)
    RevisedSimplexOptions opts;
    opts.solver_type = BasisSolverType::SPARSE_LU;
    RevisedSimplex solver(opts);
    RevisedSimplexResult rev_res = solver.solve(presolve_res.reduced_model);
    EXPECT_EQ(rev_res.status, RevisedSimplexStatus::OPTIMAL);

    // Step 3: Postsolve mapping back to Original Variable Space
    std::vector<real_t> orig_x = presolve_res.postsolve.recover_solution(rev_res.primal_solution);

    // Step 4: Verify Original Model Feasibility & Objective
    EXPECT_NEAR(orig_x[x1], 4.0, 1e-7);
    EXPECT_TRUE(presolve_res.postsolve.verify_original_feasibility(orig_x));
    real_t orig_obj = presolve_res.postsolve.compute_original_objective(orig_x);
    EXPECT_NEAR(orig_obj, rev_res.objective_value, 1e-5);
}

TEST_CASE(SparseLU_25_MandatoryLPCrossCheck) {
    // Primary Hand-Derived Benchmark LP:
    // Max 3x + 5y s.t. 2x + y <= 8, x + 2y <= 8, x, y >= 0
    // Expected solution: x = 8/3, y = 8/3, obj = 64/3 (~21.33333333)
    LPModel model("primary_hand_derived_sparse");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x = model.add_variable("x", 0.0, BHARATOPT_INFINITY, 3.0);
    index_t y = model.add_variable("y", 0.0, BHARATOPT_INFINITY, 5.0);

    model.add_constraint("c1", {{x, 2.0}, {y, 1.0}}, ConstraintSense::LESS_EQUAL, 8.0);
    model.add_constraint("c2", {{x, 1.0}, {y, 2.0}}, ConstraintSense::LESS_EQUAL, 8.0);

    RevisedSimplexOptions opts;
    opts.solver_type = BasisSolverType::SPARSE_LU;
    RevisedSimplex solver(opts);
    RevisedSimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, RevisedSimplexStatus::OPTIMAL);
    EXPECT_NEAR(res.primal_solution[x], 8.0 / 3.0, 1e-6);
    EXPECT_NEAR(res.primal_solution[y], 8.0 / 3.0, 1e-6);
    EXPECT_NEAR(res.objective_value, 64.0 / 3.0, 1e-6);
    EXPECT_TRUE(solver.verify_solution_feasibility(model, res.primal_solution));
}
