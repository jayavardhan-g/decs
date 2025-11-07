from locust import HttpUser, task, constant

class MyServerUser(HttpUser):
    wait_time = constant(0)  # No waiting between tasks

    @task(1)
    def get_value(self):
        self.client.get("/nocache/val?id=1")
"""
    @task(2)
    def save_value(self):
        self.client.post("/save?id=2&val=hello")

    @task(1)
    def delete_value(self):
        self.client.get("/delete?id=2")

    @task(1)
    def update_value(self):
        self.client.post("/put?id=3&val=updated_value")
"""
