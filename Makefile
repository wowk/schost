all: 
	$(MAKE) -C sc_host

clean:
	$(MAKE) -C sc_host clean

install:
	install -m 0777 sc_host/exe/sc_host $(INSTALLDIR)/usr/sbin/
	ln -sf /usr/sbin/sc_host $(INSTALLDIR)/usr/sbin/sc_hostd
