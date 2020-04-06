#ifndef _KEEBOOT_REQUESTS_H
#define _KEEBOOT_REQUESTS_H

namespace keeboot
{
  class start_experiment_req : public onld::start_experiment
  {
    public:
      start_experiment_req(uint8_t *mbuf, uint32_t size);
      virtual ~start_experiment_req();
 
      virtual bool handle();

    private:
      void do_kexec(std::string filesystem);
      void do_system(std::string filesystem);
  }; // class start_experiment_req

  class refresh_req : public onld::refresh
  {
    public:
      refresh_req(uint8_t *mbuf, uint32_t size);
      virtual ~refresh_req();
 
      virtual bool handle();
  }; // class refresh_req
};

#endif // _KEEBOOT_REQUESTS_H
