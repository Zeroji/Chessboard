#define RUN_MODULE(run_function) \
    extern void run_function();  \
    run_function();

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

int main(int argc, char** argv) {
    RUN_MODULE(run_fen);
    RUN_MODULE(run_castling);
    RUN_MODULE(run_promotion);
    RUN_MODULE(run_check);
    RUN_MODULE(run_moves);
}