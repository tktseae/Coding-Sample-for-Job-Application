import torch
from transformers import RobertaModel

class Model(torch.nn.Module):
  def __init__(self):
    super(Model, self).__init__()
    self.roberta = RobertaModel.from_pretrained("roberta-base")
    self.l1 = torch.nn.Linear(768, 768)
    self.dropout1 = torch.nn.Dropout(0.3)
    self.l2 = torch.nn.Linear(768, 5)
    self.softmax = torch.nn.Softmax(dim=1)

  def forward(self, input_ids, attention_mask, token_type_ids):
    roberta_output = self.roberta(input_ids=input_ids, attention_mask=attention_mask, token_type_ids=token_type_ids)
    hidden_state = roberta_output[0]
    output = hidden_state[:, 0]

    output = self.l1(output)
    output = torch.nn.functional.relu(output)
    output = self.dropout1(output)

    output = self.l2(output)
    output = self.softmax(output)

    return output