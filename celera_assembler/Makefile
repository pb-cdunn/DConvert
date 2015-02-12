KMER_DIR    =wgs-8.1/kmer
SAMTOOLS_DIR=wgs-8.1/samtools
CELERA_DIR  =wgs-8.1/src

.PHONY: kmer samtools celera clean kmer_clean samtools_clean celera_clean

all: kmer samtools celera

clean: kmer_clean samtools_clean celera_clean

kmer:
	$(MAKE) install -C $(KMER_DIR)

samtools:
	$(MAKE) -C $(SAMTOOLS_DIR)

celera:
	$(MAKE) -C $(CELERA_DIR)

kmer_clean:
	$(MAKE) clean -C $(KMER_DIR)
	rm -rf $(KMER_DIR)/Linux-amd64/lib/*.a

samtools_clean:
	$(MAKE) clean -C $(SAMTOOLS_DIR)

celera_clean:
	$(MAKE) clean -C $(CELERA_DIR)
